# SPDX-License-Identifier: GPL-3.0-or-later
"""
Unit tests for the helix_print Moonraker plugin.

These tests verify the plugin's core functionality without requiring
a running Moonraker instance. They use mocks to simulate Moonraker's
server, file manager, and history components.

Run with: pytest tests/test_helix_print.py -v
"""

import asyncio
import json
import os
import tempfile
from pathlib import Path
from typing import Any, Dict, Optional
from unittest.mock import AsyncMock, MagicMock, patch

import pytest

# Import the plugin (adjust path as needed)
import sys
sys.path.insert(0, str(Path(__file__).parent.parent))

from helix_print import HelixPrint, PrintInfo, load_component


# ============================================================================
# Test Fixtures and Mocks
# ============================================================================

class MockWebRequest:
    """Mock WebRequest for testing API endpoints."""

    def __init__(self, params: Dict[str, Any]):
        self._params = params

    def get_str(self, key: str, default: str = "") -> str:
        return str(self._params.get(key, default))

    def get_list(self, key: str, default: list = None) -> list:
        return self._params.get(key, default or [])

    def get_boolean(self, key: str, default: bool = False) -> bool:
        return bool(self._params.get(key, default))


class MockServer:
    """Mock Moonraker server for testing."""

    def __init__(self):
        self.endpoints = {}
        self.event_handlers = {}
        self.components = {}
        self._error_class = Exception

    def register_endpoint(self, path: str, methods: list, handler):
        self.endpoints[path] = handler

    def register_event_handler(self, event: str, callback):
        if event not in self.event_handlers:
            self.event_handlers[event] = []
        self.event_handlers[event].append(callback)

    def lookup_component(self, name: str, default=None):
        return self.components.get(name, default)

    def get_event_loop(self):
        return MockEventLoop()

    def error(self, message: str, code: int = 500):
        return Exception(f"{code}: {message}")


class MockEventLoop:
    """Mock event loop for testing."""

    def register_callback(self, callback, *args):
        pass

    def delay_callback(self, delay: float, callback, *args):
        pass


class MockFileManager:
    """Mock file manager for testing."""

    def __init__(self, gcodes_path: str):
        self._gcodes_path = gcodes_path

    def get_directory(self, name: str) -> str:
        if name == "gcodes":
            return self._gcodes_path
        return ""


class MockDatabase:
    """Mock database for testing."""

    def __init__(self):
        self.data = {}
        self.tables_created = []

    async def execute_db_command(self, sql: str, params: tuple = None):
        if sql.strip().upper().startswith("CREATE TABLE"):
            self.tables_created.append(sql)
        return MagicMock(lastrowid=1)


class MockKlippy:
    """Mock Klipper connection for testing."""

    def __init__(self):
        self.commands_sent = []

    async def run_gcode(self, gcode: str):
        self.commands_sent.append(gcode)


class MockHistory:
    """Mock history component for testing."""

    def __init__(self):
        self.jobs = {}
        self.modifications = []

    async def get_job(self, job_id: str):
        return self.jobs.get(job_id)

    async def modify_job(self, job_id: str, **kwargs):
        self.modifications.append({"job_id": job_id, **kwargs})


class MockConfigHelper:
    """Mock config helper for testing."""

    def __init__(self, server: MockServer, options: Dict[str, Any] = None):
        self._server = server
        self._options = options or {}

    def get_server(self):
        return self._server

    def get(self, key: str, default: str = None) -> str:
        return self._options.get(key, default)

    def getint(self, key: str, default: int = None) -> int:
        return int(self._options.get(key, default))

    def getboolean(self, key: str, default: bool = None) -> bool:
        return bool(self._options.get(key, default))


@pytest.fixture
def temp_gcodes_dir():
    """Create a temporary directory for G-code files."""
    with tempfile.TemporaryDirectory() as tmpdir:
        yield tmpdir


@pytest.fixture
def mock_server():
    """Create a mock Moonraker server."""
    return MockServer()


@pytest.fixture
def helix_print_component(mock_server, temp_gcodes_dir):
    """Create a HelixPrint component instance for testing."""
    # Set up mock components
    mock_server.components["file_manager"] = MockFileManager(temp_gcodes_dir)
    mock_server.components["database"] = MockDatabase()
    mock_server.components["klippy_connection"] = MockKlippy()
    mock_server.components["history"] = MockHistory()

    # Create config
    config = MockConfigHelper(mock_server, {
        "temp_dir": ".helix_temp",
        "symlink_dir": ".helix_print",
        "cleanup_delay": 3600,
        "enabled": True,
    })

    # Create component
    component = load_component(config)
    return component


# ============================================================================
# PrintInfo Tests
# ============================================================================

class TestPrintInfo:
    """Tests for the PrintInfo data class."""

    def test_creation(self):
        """Test PrintInfo can be created with all fields."""
        info = PrintInfo(
            original_filename="benchy.gcode",
            temp_filename=".helix_temp/mod_123_benchy.gcode",
            symlink_filename=".helix_print/benchy.gcode",
            modifications=["bed_leveling_disabled"],
            start_time=1234567890.0,
        )

        assert info.original_filename == "benchy.gcode"
        assert info.temp_filename == ".helix_temp/mod_123_benchy.gcode"
        assert info.symlink_filename == ".helix_print/benchy.gcode"
        assert info.modifications == ["bed_leveling_disabled"]
        assert info.start_time == 1234567890.0
        assert info.job_id is None
        assert info.db_id is None

    def test_job_id_assignment(self):
        """Test job_id can be assigned after creation."""
        info = PrintInfo(
            original_filename="test.gcode",
            temp_filename="temp.gcode",
            symlink_filename="symlink.gcode",
            modifications=[],
            start_time=0.0,
        )

        info.job_id = "ABC123"
        assert info.job_id == "ABC123"


# ============================================================================
# Component Initialization Tests
# ============================================================================

class TestHelixPrintInit:
    """Tests for HelixPrint component initialization."""

    def test_load_component(self, mock_server):
        """Test component loads successfully."""
        config = MockConfigHelper(mock_server)
        component = load_component(config)

        assert component is not None
        assert isinstance(component, HelixPrint)

    def test_default_config(self, mock_server):
        """Test default configuration values."""
        config = MockConfigHelper(mock_server)
        component = load_component(config)

        assert component.temp_dir == ".helix_temp"
        assert component.symlink_dir == ".helix_print"
        assert component.cleanup_delay == 86400  # 24 hours
        assert component.enabled is True

    def test_custom_config(self, mock_server):
        """Test custom configuration values."""
        config = MockConfigHelper(mock_server, {
            "temp_dir": "custom_temp",
            "symlink_dir": "custom_symlink",
            "cleanup_delay": 7200,
            "enabled": False,
        })
        component = load_component(config)

        assert component.temp_dir == "custom_temp"
        assert component.symlink_dir == "custom_symlink"
        assert component.cleanup_delay == 7200
        assert component.enabled is False

    def test_endpoints_registered(self, mock_server):
        """Test API endpoints are registered."""
        config = MockConfigHelper(mock_server)
        load_component(config)

        assert "/server/helix/print_modified" in mock_server.endpoints
        assert "/server/helix/status" in mock_server.endpoints

    def test_event_handlers_registered(self, mock_server):
        """Test event handlers are registered."""
        config = MockConfigHelper(mock_server)
        load_component(config)

        assert "job_state:state_changed" in mock_server.event_handlers
        assert "server:klippy_ready" in mock_server.event_handlers


# ============================================================================
# Status API Tests
# ============================================================================

class TestStatusAPI:
    """Tests for the /server/helix/status endpoint."""

    @pytest.mark.asyncio
    async def test_status_returns_config(self, helix_print_component, mock_server):
        """Test status endpoint returns configuration."""
        handler = mock_server.endpoints["/server/helix/status"]
        request = MockWebRequest({})

        result = await handler(request)

        assert result["enabled"] is True
        assert result["temp_dir"] == ".helix_temp"
        assert result["symlink_dir"] == ".helix_print"
        assert result["cleanup_delay"] == 3600
        assert result["version"] == "1.0.0"
        assert result["active_prints"] == 0


# ============================================================================
# Print Modified API Tests
# ============================================================================

class TestPrintModifiedAPI:
    """Tests for the /server/helix/print_modified endpoint."""

    @pytest.mark.asyncio
    async def test_rejects_missing_original(self, helix_print_component, mock_server,
                                            temp_gcodes_dir):
        """Test API rejects request when original file doesn't exist."""
        # Initialize component
        await helix_print_component.component_init()

        handler = mock_server.endpoints["/server/helix/print_modified"]
        request = MockWebRequest({
            "original_filename": "nonexistent.gcode",
            "modified_content": "G28\n",
            "modifications": [],
        })

        with pytest.raises(Exception) as exc_info:
            await handler(request)

        assert "not found" in str(exc_info.value).lower()

    @pytest.mark.asyncio
    async def test_creates_temp_file(self, helix_print_component, mock_server,
                                     temp_gcodes_dir):
        """Test API creates temp file with modified content."""
        # Create original file
        original = Path(temp_gcodes_dir) / "benchy.gcode"
        original.write_text("G28\nBED_MESH_CALIBRATE\nG1 X0 Y0\n")

        # Initialize component
        await helix_print_component.component_init()

        handler = mock_server.endpoints["/server/helix/print_modified"]
        request = MockWebRequest({
            "original_filename": "benchy.gcode",
            "modified_content": "G28\n; BED_MESH_CALIBRATE disabled\nG1 X0 Y0\n",
            "modifications": ["bed_leveling_disabled"],
        })

        result = await handler(request)

        assert result["original_filename"] == "benchy.gcode"
        assert result["status"] == "printing"
        assert ".helix_temp" in result["temp_filename"]

        # Verify temp file was created
        temp_path = Path(temp_gcodes_dir) / result["temp_filename"]
        assert temp_path.exists()
        assert "BED_MESH_CALIBRATE disabled" in temp_path.read_text()

    @pytest.mark.asyncio
    async def test_creates_symlink(self, helix_print_component, mock_server,
                                   temp_gcodes_dir):
        """Test API creates symlink to temp file."""
        # Create original file
        original = Path(temp_gcodes_dir) / "benchy.gcode"
        original.write_text("G28\n")

        # Initialize component
        await helix_print_component.component_init()

        handler = mock_server.endpoints["/server/helix/print_modified"]
        request = MockWebRequest({
            "original_filename": "benchy.gcode",
            "modified_content": "G28\n",
            "modifications": [],
        })

        result = await handler(request)

        # Verify symlink was created
        symlink_path = Path(temp_gcodes_dir) / result["print_filename"]
        assert symlink_path.is_symlink()

    @pytest.mark.asyncio
    async def test_starts_print_with_symlink(self, helix_print_component, mock_server,
                                             temp_gcodes_dir):
        """Test API starts print using symlink path."""
        # Create original file
        original = Path(temp_gcodes_dir) / "benchy.gcode"
        original.write_text("G28\n")

        # Initialize component
        await helix_print_component.component_init()

        handler = mock_server.endpoints["/server/helix/print_modified"]
        request = MockWebRequest({
            "original_filename": "benchy.gcode",
            "modified_content": "G28\n",
            "modifications": [],
        })

        await handler(request)

        # Verify print command was sent
        klippy = mock_server.components["klippy_connection"]
        assert len(klippy.commands_sent) == 1
        assert ".helix_print/benchy.gcode" in klippy.commands_sent[0]

    @pytest.mark.asyncio
    async def test_disabled_returns_error(self, mock_server, temp_gcodes_dir):
        """Test API returns error when component is disabled."""
        mock_server.components["file_manager"] = MockFileManager(temp_gcodes_dir)
        mock_server.components["database"] = MockDatabase()

        config = MockConfigHelper(mock_server, {"enabled": False})
        component = load_component(config)

        handler = mock_server.endpoints["/server/helix/print_modified"]
        request = MockWebRequest({
            "original_filename": "test.gcode",
            "modified_content": "G28\n",
        })

        with pytest.raises(Exception) as exc_info:
            await handler(request)

        assert "disabled" in str(exc_info.value).lower()


# ============================================================================
# Symlink Conflict Tests
# ============================================================================

class TestSymlinkConflicts:
    """Tests for symlink conflict handling."""

    @pytest.mark.asyncio
    async def test_replaces_existing_symlink(self, helix_print_component, mock_server,
                                             temp_gcodes_dir):
        """Test that existing symlinks are replaced."""
        # Create original file
        original = Path(temp_gcodes_dir) / "benchy.gcode"
        original.write_text("G28\n")

        # Create existing symlink
        symlink_dir = Path(temp_gcodes_dir) / ".helix_print"
        symlink_dir.mkdir(parents=True, exist_ok=True)
        existing_symlink = symlink_dir / "benchy.gcode"
        existing_symlink.symlink_to("/nonexistent")

        # Initialize component
        await helix_print_component.component_init()

        handler = mock_server.endpoints["/server/helix/print_modified"]
        request = MockWebRequest({
            "original_filename": "benchy.gcode",
            "modified_content": "G28\n",
            "modifications": [],
        })

        # Should succeed, replacing the existing symlink
        result = await handler(request)
        assert result["status"] == "printing"


# ============================================================================
# Active Print Tracking Tests
# ============================================================================

class TestActivePrintTracking:
    """Tests for active print tracking."""

    @pytest.mark.asyncio
    async def test_tracks_active_print(self, helix_print_component, mock_server,
                                       temp_gcodes_dir):
        """Test that active prints are tracked."""
        # Create original file
        original = Path(temp_gcodes_dir) / "benchy.gcode"
        original.write_text("G28\n")

        # Initialize component
        await helix_print_component.component_init()

        handler = mock_server.endpoints["/server/helix/print_modified"]
        request = MockWebRequest({
            "original_filename": "benchy.gcode",
            "modified_content": "G28\n",
            "modifications": ["test_mod"],
        })

        result = await handler(request)

        # Check active prints
        assert len(helix_print_component.active_prints) == 1
        print_info = helix_print_component.active_prints[result["print_filename"]]
        assert print_info.original_filename == "benchy.gcode"
        assert print_info.modifications == ["test_mod"]


# ============================================================================
# Path Validation Tests
# ============================================================================

class TestPathValidation:
    """Tests for path validation and security."""

    @pytest.mark.asyncio
    async def test_handles_subdirectory_path(self, helix_print_component, mock_server,
                                             temp_gcodes_dir):
        """Test handling of files in subdirectories."""
        # Create subdirectory and file
        subdir = Path(temp_gcodes_dir) / "prints" / "2024"
        subdir.mkdir(parents=True, exist_ok=True)
        original = subdir / "benchy.gcode"
        original.write_text("G28\n")

        # Initialize component
        await helix_print_component.component_init()

        handler = mock_server.endpoints["/server/helix/print_modified"]
        request = MockWebRequest({
            "original_filename": "prints/2024/benchy.gcode",
            "modified_content": "G28\n",
            "modifications": [],
        })

        result = await handler(request)
        assert result["status"] == "printing"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
