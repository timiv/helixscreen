# SPDX-License-Identifier: GPL-3.0-or-later
# HelixScreen KIAUH Extension
#
# This extension integrates HelixScreen with KIAUH for easy installation,
# updates, and removal through the KIAUH menu system.

from __future__ import annotations

from pathlib import Path

# Module path for asset resolution
MODULE_PATH = Path(__file__).parent

# HelixScreen configuration
HELIXSCREEN_REPO = "https://github.com/prestonbrown/helixscreen"
HELIXSCREEN_DIR = Path.home() / "helixscreen"
HELIXSCREEN_SERVICE_NAME = "helixscreen"

# Static install locations (platform-dependent)
_STATIC_INSTALL_PATHS = [
    Path.home() / "helixscreen",           # Pi (current user)
    Path("/opt/helixscreen"),              # Pi (fallback), AD5M Forge-X
    Path("/usr/data/helixscreen"),         # K1/Simple AF
    Path("/root/printer_software/helixscreen"),  # AD5M Klipper Mod
]
HELIXSCREEN_INSTALLER_URL = (
    "https://raw.githubusercontent.com/prestonbrown/helixscreen/main/scripts/install.sh"
)


def _get_install_paths() -> list[Path]:
    """Build install paths list, including all /home/*/helixscreen dirs."""
    paths = list(_STATIC_INSTALL_PATHS)
    # Scan for installs under any user's home directory
    home = Path("/home")
    if home.is_dir():
        for home_dir in home.iterdir():
            candidate = home_dir / "helixscreen"
            if candidate not in paths:
                paths.insert(0, candidate)
    return paths


# Exported for backward compatibility
HELIXSCREEN_INSTALL_PATHS = _get_install_paths()


def find_install_dir() -> Path | None:
    """Find the actual HelixScreen install directory."""
    for path in HELIXSCREEN_INSTALL_PATHS:
        if path.exists() and (path / "helix-screen").exists():
            return path
    return None
