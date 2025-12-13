# SPDX-License-Identifier: GPL-3.0-or-later
"""
HelixPrint - Moonraker component for handling modified G-code files.

This component provides a single API endpoint that handles the complete workflow
for printing modified G-code while preserving original file attribution in
Klipper's print_stats and Moonraker's history.

Key features:
- Single API endpoint: POST /server/helix/print_modified
- Symlink-based filename preservation (Klipper sees original name)
- Automatic history patching to record original filename
- Configurable cleanup of temporary files

Configuration (moonraker.conf):
    [helix_print]
    enabled: True
    temp_dir: .helix_temp
    symlink_dir: .helix_print
    cleanup_delay: 86400
"""

from __future__ import annotations

import asyncio
import json
import logging
import os
import shutil
import time
from pathlib import Path
from typing import TYPE_CHECKING, Any, Dict, List, Optional

if TYPE_CHECKING:
    from moonraker.common import RequestType, WebRequest
    from moonraker.confighelper import ConfigHelper
    from moonraker.server import Server

# Database table name for tracking temp files
HELIX_TEMP_TABLE = "helix_temp_files"


class PrintInfo:
    """Tracks information about an active modified print."""

    def __init__(
        self,
        original_filename: str,
        temp_filename: str,
        symlink_filename: str,
        modifications: List[str],
        start_time: float,
    ) -> None:
        self.original_filename = original_filename
        self.temp_filename = temp_filename
        self.symlink_filename = symlink_filename
        self.modifications = modifications
        self.start_time = start_time
        self.job_id: Optional[str] = None
        self.db_id: Optional[int] = None


class HelixPrint:
    """
    Moonraker component for handling modified G-code files.

    Provides:
    - Single API endpoint for modified print workflow
    - Symlink-based filename preservation for print_stats
    - History patching to record original filename
    - Automatic cleanup of temp files
    """

    def __init__(self, config: ConfigHelper) -> None:
        self.server: Server = config.get_server()
        self.eventloop = self.server.get_event_loop()

        # Configuration options
        self.temp_dir = config.get("temp_dir", ".helix_temp")
        self.symlink_dir = config.get("symlink_dir", ".helix_print")
        self.cleanup_delay = config.getint("cleanup_delay", 86400)  # 24 hours
        self.enabled = config.getboolean("enabled", True)

        # Component references (resolved after init)
        self.file_manager: Optional[Any] = None
        self.history: Optional[Any] = None
        self.klippy: Optional[Any] = None
        self.database: Optional[Any] = None

        # State tracking
        self.active_prints: Dict[str, PrintInfo] = {}
        self.gc_path: Optional[Path] = None

        # Register API endpoints
        self.server.register_endpoint(
            "/server/helix/print_modified",
            ["POST"],
            self._handle_print_modified,
        )
        self.server.register_endpoint(
            "/server/helix/status",
            ["GET"],
            self._handle_status,
        )

        # Register event handlers
        self.server.register_event_handler(
            "job_state:state_changed", self._on_job_state_changed
        )
        self.server.register_event_handler(
            "server:klippy_ready", self._on_klippy_ready
        )

        logging.info(
            f"HelixPrint initialized: temp={self.temp_dir}, "
            f"symlink={self.symlink_dir}, cleanup={self.cleanup_delay}s"
        )

    async def component_init(self) -> None:
        """Called after all components are loaded."""
        self.file_manager = self.server.lookup_component("file_manager")
        self.history = self.server.lookup_component("history", None)
        self.klippy = self.server.lookup_component("klippy_connection")
        self.database = self.server.lookup_component("database")

        # Get gcodes path
        self.gc_path = Path(self.file_manager.get_directory("gcodes"))

        # Ensure directories exist
        await self._ensure_directories()

        # Initialize database table
        await self._init_database()

        # Schedule startup cleanup
        self.eventloop.register_callback(self._startup_cleanup)

    async def _ensure_directories(self) -> None:
        """Ensure temp and symlink directories exist."""
        if self.gc_path is None:
            return

        temp_path = self.gc_path / self.temp_dir
        symlink_path = self.gc_path / self.symlink_dir

        temp_path.mkdir(parents=True, exist_ok=True)
        symlink_path.mkdir(parents=True, exist_ok=True)

        logging.debug(f"HelixPrint: Ensured directories exist: {temp_path}, {symlink_path}")

    async def _init_database(self) -> None:
        """Initialize database table for tracking temp files."""
        if self.database is None:
            logging.warning("HelixPrint: Database not available, persistence disabled")
            return

        # Create table if it doesn't exist
        await self.database.execute_db_command(
            f"""
            CREATE TABLE IF NOT EXISTS {HELIX_TEMP_TABLE} (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                original_filename TEXT NOT NULL,
                temp_filename TEXT NOT NULL,
                symlink_filename TEXT NOT NULL,
                modifications TEXT,
                job_id TEXT,
                created_at REAL NOT NULL,
                cleanup_scheduled_at REAL,
                status TEXT DEFAULT 'active'
            )
            """
        )
        logging.debug("HelixPrint: Database table initialized")

    async def _handle_status(self, web_request: WebRequest) -> Dict[str, Any]:
        """Handle status request - useful for plugin detection."""
        return {
            "enabled": self.enabled,
            "temp_dir": self.temp_dir,
            "symlink_dir": self.symlink_dir,
            "cleanup_delay": self.cleanup_delay,
            "active_prints": len(self.active_prints),
            "version": "1.0.0",
        }

    async def _handle_print_modified(self, web_request: WebRequest) -> Dict[str, Any]:
        """
        Handle the print_modified API request.

        This is the main entry point for printing modified G-code files.
        It handles the complete workflow:
        1. Validate original file exists
        2. Save modified content to temp directory
        3. Copy metadata from original
        4. Create symlink with original filename
        5. Start print via symlink
        """
        if not self.enabled:
            raise self.server.error("HelixPrint component is disabled", 503)

        if self.gc_path is None:
            raise self.server.error("File manager not initialized", 500)

        # Validate required parameters
        original_filename = web_request.get_str("original_filename")
        modified_content = web_request.get_str("modified_content")
        modifications = web_request.get_list("modifications", [])
        copy_metadata = web_request.get_boolean("copy_metadata", True)

        # Validate original file exists
        original_path = self.gc_path / original_filename
        if not original_path.exists():
            raise self.server.error(
                f"Original file not found: {original_filename}", 400
            )

        # Generate temp filename with timestamp
        timestamp = int(time.time())
        base_name = Path(original_filename).name
        temp_filename = f"{self.temp_dir}/mod_{timestamp}_{base_name}"
        temp_path = self.gc_path / temp_filename

        # Ensure temp directory exists
        temp_path.parent.mkdir(parents=True, exist_ok=True)

        # Write modified content
        try:
            temp_path.write_text(modified_content, encoding="utf-8")
            logging.info(f"HelixPrint: Wrote modified content to {temp_filename}")
        except Exception as e:
            raise self.server.error(f"Failed to write temp file: {e}", 500)

        # Copy metadata (thumbnails) from original
        if copy_metadata:
            await self._copy_metadata(original_path, temp_path)

        # Create symlink with original filename
        symlink_filename = f"{self.symlink_dir}/{base_name}"
        symlink_path = self.gc_path / symlink_filename

        # Handle symlink conflicts
        if symlink_path.exists() or symlink_path.is_symlink():
            symlink_path.unlink()
            logging.debug(f"HelixPrint: Removed existing symlink at {symlink_path}")

        symlink_path.parent.mkdir(parents=True, exist_ok=True)

        try:
            symlink_path.symlink_to(temp_path)
            logging.info(f"HelixPrint: Created symlink {symlink_filename} -> {temp_filename}")
        except Exception as e:
            # Clean up temp file on symlink failure
            temp_path.unlink(missing_ok=True)
            raise self.server.error(f"Failed to create symlink: {e}", 500)

        # Track this print
        print_info = PrintInfo(
            original_filename=original_filename,
            temp_filename=temp_filename,
            symlink_filename=symlink_filename,
            modifications=modifications,
            start_time=time.time(),
        )
        self.active_prints[symlink_filename] = print_info

        # Persist to database for crash recovery
        await self._persist_print_info(print_info)

        # Start the print with symlink path
        try:
            await self.klippy.run_gcode(f'SDCARD_PRINT_FILE FILENAME="{symlink_filename}"')
            logging.info(f"HelixPrint: Started print with {symlink_filename}")
        except Exception as e:
            # Clean up on print start failure
            symlink_path.unlink(missing_ok=True)
            temp_path.unlink(missing_ok=True)
            del self.active_prints[symlink_filename]
            raise self.server.error(f"Failed to start print: {e}", 500)

        return {
            "original_filename": original_filename,
            "print_filename": symlink_filename,
            "temp_filename": temp_filename,
            "status": "printing",
        }

    async def _copy_metadata(self, original_path: Path, temp_path: Path) -> None:
        """Copy slicer metadata (thumbnails) from original to temp file."""
        if self.gc_path is None:
            return

        thumbs_dir = self.gc_path / ".thumbs"
        if not thumbs_dir.exists():
            return

        original_stem = original_path.stem
        temp_stem = temp_path.stem

        # Find and link thumbnails for the original file
        for thumb in thumbs_dir.glob(f"{original_stem}*"):
            try:
                # Create symlink to original thumbnail with new name
                new_name = thumb.name.replace(original_stem, temp_stem)
                temp_thumb = thumbs_dir / new_name
                if not temp_thumb.exists():
                    temp_thumb.symlink_to(thumb)
                    logging.debug(f"HelixPrint: Linked thumbnail {new_name} -> {thumb.name}")
            except Exception as e:
                logging.warning(f"HelixPrint: Failed to link thumbnail: {e}")

    async def _persist_print_info(self, print_info: PrintInfo) -> None:
        """Save print info to database for crash recovery."""
        if self.database is None:
            return

        try:
            result = await self.database.execute_db_command(
                f"""
                INSERT INTO {HELIX_TEMP_TABLE}
                (original_filename, temp_filename, symlink_filename,
                 modifications, created_at, status)
                VALUES (?, ?, ?, ?, ?, ?)
                """,
                (
                    print_info.original_filename,
                    print_info.temp_filename,
                    print_info.symlink_filename,
                    json.dumps(print_info.modifications),
                    time.time(),
                    "active",
                ),
            )
            print_info.db_id = result.lastrowid
        except Exception as e:
            logging.warning(f"HelixPrint: Failed to persist print info: {e}")

    async def _on_klippy_ready(self) -> None:
        """Handle Klipper ready event - recover from any interrupted prints."""
        logging.debug("HelixPrint: Klipper ready, checking for interrupted prints")
        # Recovery logic would go here if needed

    async def _on_job_state_changed(
        self,
        job_event: Any,
        prev_stats: Dict[str, Any],
        new_stats: Dict[str, Any],
    ) -> None:
        """Handle job state changes to patch history."""
        state = new_stats.get("state", "")
        filename = new_stats.get("filename", "")

        # Check if this is one of our modified prints
        if not filename.startswith(f"{self.symlink_dir}/"):
            return

        print_info = self.active_prints.get(filename)
        if not print_info:
            logging.warning(f"HelixPrint: Unknown modified file: {filename}")
            return

        # Capture job_id when print starts
        if state == "printing":
            job_id = new_stats.get("job_id")
            if job_id:
                print_info.job_id = job_id
                logging.info(f"HelixPrint: Job started with ID {job_id}")

        # Handle completion states
        if state in ("complete", "cancelled", "error"):
            logging.info(f"HelixPrint: Job finished ({state}): {filename}")

            # Patch history entry
            if self.history is not None:
                await self._patch_history_entry(print_info, state)

            # Schedule cleanup
            await self._schedule_cleanup(print_info)

            # Remove from active tracking
            del self.active_prints[filename]

    async def _patch_history_entry(
        self, print_info: PrintInfo, final_state: str
    ) -> None:
        """Patch the history entry to show original filename."""
        if not self.history or not print_info.job_id:
            return

        try:
            # Get the job from history
            job = await self.history.get_job(print_info.job_id)
            if not job:
                logging.warning(
                    f"HelixPrint: Job {print_info.job_id} not in history"
                )
                return

            # Extract original filename (strip symlink dir prefix if present)
            original = print_info.original_filename
            if original.startswith(f"{self.symlink_dir}/"):
                original = original[len(self.symlink_dir) + 1 :]

            # Update auxiliary_data with modification info
            aux_data = job.get("auxiliary_data", {}) or {}
            aux_data["helix_modifications"] = print_info.modifications
            aux_data["helix_temp_file"] = print_info.temp_filename
            aux_data["helix_symlink"] = print_info.symlink_filename
            aux_data["helix_original"] = print_info.original_filename

            # Update the history entry
            # Note: This may need adjustment based on actual Moonraker history API
            await self.history.modify_job(
                print_info.job_id,
                filename=original,
                auxiliary_data=aux_data,
            )

            logging.info(
                f"HelixPrint: Patched history {print_info.job_id} "
                f"filename to '{original}'"
            )

        except Exception as e:
            logging.exception(f"HelixPrint: Failed to patch history: {e}")

    async def _schedule_cleanup(self, print_info: PrintInfo) -> None:
        """Schedule cleanup of temp files after delay."""
        if self.gc_path is None:
            return

        # Immediately delete symlink (no longer needed)
        symlink_path = self.gc_path / print_info.symlink_filename
        if symlink_path.is_symlink():
            symlink_path.unlink()
            logging.debug(f"HelixPrint: Removed symlink {symlink_path}")

        # Also clean up thumbnail symlinks
        await self._cleanup_thumbnail_symlinks(print_info.temp_filename)

        # Update database status
        if self.database is not None:
            cleanup_time = time.time() + self.cleanup_delay
            try:
                await self.database.execute_db_command(
                    f"""
                    UPDATE {HELIX_TEMP_TABLE}
                    SET cleanup_scheduled_at = ?, status = ?
                    WHERE temp_filename = ?
                    """,
                    (cleanup_time, "pending_cleanup", print_info.temp_filename),
                )
            except Exception as e:
                logging.warning(f"HelixPrint: Failed to update cleanup status: {e}")

        # Schedule delayed cleanup
        self.eventloop.delay_callback(
            self.cleanup_delay,
            self._cleanup_temp_file,
            print_info.temp_filename,
        )

        logging.info(
            f"HelixPrint: Scheduled cleanup of {print_info.temp_filename} "
            f"in {self.cleanup_delay}s"
        )

    async def _cleanup_thumbnail_symlinks(self, temp_filename: str) -> None:
        """Clean up thumbnail symlinks for a temp file."""
        if self.gc_path is None:
            return

        thumbs_dir = self.gc_path / ".thumbs"
        if not thumbs_dir.exists():
            return

        temp_stem = Path(temp_filename).stem
        for thumb in thumbs_dir.glob(f"{temp_stem}*"):
            if thumb.is_symlink():
                thumb.unlink()
                logging.debug(f"HelixPrint: Removed thumbnail symlink {thumb}")

    async def _cleanup_temp_file(self, temp_filename: str) -> None:
        """Delete a temp file after cleanup delay."""
        if self.gc_path is None:
            return

        temp_path = self.gc_path / temp_filename
        if temp_path.exists():
            temp_path.unlink()
            logging.info(f"HelixPrint: Cleaned up {temp_filename}")

        # Update database
        if self.database is not None:
            try:
                await self.database.execute_db_command(
                    f"""
                    UPDATE {HELIX_TEMP_TABLE}
                    SET status = ?
                    WHERE temp_filename = ?
                    """,
                    ("cleaned", temp_filename),
                )
            except Exception as e:
                logging.warning(f"HelixPrint: Failed to update cleanup status: {e}")

    async def _startup_cleanup(self) -> None:
        """Clean up stale temp files on startup."""
        if self.gc_path is None or self.database is None:
            return

        now = time.time()

        try:
            # Find files past their cleanup time
            rows = await self.database.execute_db_command(
                f"""
                SELECT temp_filename, symlink_filename
                FROM {HELIX_TEMP_TABLE}
                WHERE status = 'pending_cleanup' AND cleanup_scheduled_at < ?
                """,
                (now,),
            )

            if not rows:
                return

            for row in rows:
                temp_filename = row["temp_filename"]
                symlink_filename = row["symlink_filename"]

                # Clean up files
                temp_path = self.gc_path / temp_filename
                symlink_path = self.gc_path / symlink_filename

                if temp_path.exists():
                    temp_path.unlink()
                if symlink_path.is_symlink():
                    symlink_path.unlink()

                # Clean up thumbnail symlinks
                await self._cleanup_thumbnail_symlinks(temp_filename)

                # Update status
                await self.database.execute_db_command(
                    f"""
                    UPDATE {HELIX_TEMP_TABLE}
                    SET status = ?
                    WHERE temp_filename = ?
                    """,
                    ("cleaned", temp_filename),
                )

            logging.info(f"HelixPrint: Startup cleanup removed {len(rows)} stale files")

        except Exception as e:
            logging.exception(f"HelixPrint: Startup cleanup failed: {e}")


def load_component(config: ConfigHelper) -> HelixPrint:
    """Factory function to load the HelixPrint component."""
    return HelixPrint(config)
