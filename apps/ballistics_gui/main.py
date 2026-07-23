#!/usr/bin/env python3
"""Entry point for the local Phase Two desktop range."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys


def _repository_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Native fictional ballistics range")
    parser.add_argument("--smoke-test", action="store_true", help="run one shot and exit")
    parser.add_argument("--screenshot", type=Path, help="save Panda3D framebuffer in smoke mode")
    parser.add_argument("--window-screenshot", type=Path, help="save complete desktop window in smoke mode")
    parser.add_argument("--smoke-scope", action="store_true", help="raise scope during smoke test")
    parser.add_argument(
        "--cli",
        type=Path,
        default=_repository_root() / "build/apps/ballistic_cli/ballistics_cli",
        help="path to the built C simulation CLI",
    )
    return parser.parse_args()


def main() -> int:
    arguments = _arguments()
    cli_path = arguments.cli.resolve()
    if not cli_path.is_file():
        print(
            f"ballistics_gui: CLI not found at {cli_path}; run ./scripts/build_debug.sh first",
            file=sys.stderr,
        )
        return 2
    try:
        from PyQt6.QtCore import QTimer
        from PyQt6.QtWidgets import QApplication
    except ImportError:
        print(
            "ballistics_gui: PyQt6 is not installed; run ./scripts/setup_gui.sh",
            file=sys.stderr,
        )
        return 2

    repository_root = _repository_root()
    if str(repository_root) not in sys.path:
        sys.path.insert(0, str(repository_root))

    from apps.ballistics_gui.ui.main_window import MainWindow
    from apps.ballistics_gui.ui.style import APP_STYLE

    application = QApplication(sys.argv)
    application.setApplicationName("Ballistics Range Lab")
    application.setOrganizationName("Ballistics Research")
    application.setStyleSheet(APP_STYLE)
    window = MainWindow(cli_path)
    window.show()
    if arguments.smoke_test:
        QTimer.singleShot(250, window.fire)
        if arguments.smoke_scope:
            QTimer.singleShot(500, window.viewport.toggle_scope)
        if arguments.screenshot is not None:
            screenshot_path = arguments.screenshot.resolve()
            QTimer.singleShot(1800, lambda: window.save_viewport_screenshot(screenshot_path))
        if arguments.window_screenshot is not None:
            window_screenshot_path = arguments.window_screenshot.resolve()
            QTimer.singleShot(1850, lambda: window.save_window_screenshot(window_screenshot_path))
        QTimer.singleShot(2200, application.quit)
    return application.exec()


if __name__ == "__main__":
    raise SystemExit(main())
