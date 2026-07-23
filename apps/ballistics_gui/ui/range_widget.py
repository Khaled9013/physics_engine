"""Qt host for Panda3D's native X11 child window."""

from __future__ import annotations

from pathlib import Path

from PyQt6.QtCore import QTimer, pyqtSignal, pyqtSlot
from PyQt6.QtWidgets import QFrame
from panda3d.core import NativeWindowHandle, WindowProperties, loadPrcFileData

from ..simulation.models import ShotResult
from ..render.range_scene import RangeScene
from ..render.scoring import TargetScore


class RangeWidget(QFrame):
    fire_requested = pyqtSignal()
    aim_changed = pyqtSignal(float, float)
    scope_changed = pyqtSignal(bool)
    renderer_ready = pyqtSignal()
    renderer_failed = pyqtSignal(str)

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setObjectName("rangeViewport")
        self.setMinimumSize(640, 480)
        self.base = None
        self.scene: RangeScene | None = None
        self.render_timer = QTimer(self)
        self.render_timer.setInterval(16)
        self.render_timer.timeout.connect(self._step_renderer)
        QTimer.singleShot(0, self._initialize_renderer)

    @pyqtSlot()
    def _initialize_renderer(self) -> None:
        try:
            loadPrcFileData("", "window-type none")
            loadPrcFileData("", "audio-library-name null")
            loadPrcFileData("", "sync-video false")
            loadPrcFileData("", "framebuffer-multisample 1")
            loadPrcFileData("", "multisamples 4")
            loadPrcFileData("", "threading-model Cull/Draw")
            loadPrcFileData("", "notify-level warning")
            from direct.showbase.ShowBase import ShowBase

            self.winId()
            self.base = ShowBase(windowType="none")
            properties = WindowProperties()
            properties.setParentWindow(NativeWindowHandle.makeInt(int(self.winId())))
            properties.setOrigin(0, 0)
            properties.setSize(max(self.width(), 1), max(self.height(), 1))
            if not self.base.openDefaultWindow(props=properties):
                raise RuntimeError("Panda3D could not open an embedded graphics window")
            self.scene = RangeScene(
                self.base,
                self.fire_requested.emit,
                self.aim_changed.emit,
                self.scope_changed.emit,
            )
            self.render_timer.start()
            self.renderer_ready.emit()
        except Exception as error:
            self.renderer_failed.emit(str(error))

    @pyqtSlot()
    def _step_renderer(self) -> None:
        if self.base is not None:
            self.base.taskMgr.step()

    def resizeEvent(self, event) -> None:  # type: ignore[no-untyped-def]
        super().resizeEvent(event)
        if self.base is not None and self.base.win is not None:
            properties = WindowProperties()
            properties.setOrigin(0, 0)
            properties.setSize(max(self.width(), 1), max(self.height(), 1))
            self.base.win.requestProperties(properties)

    def set_aim(self, elevation_deg: float, azimuth_deg: float) -> None:
        if self.scene is not None:
            self.scene.set_aim(elevation_deg, azimuth_deg)

    def toggle_scope(self) -> None:
        if self.scene is not None:
            self.scene.toggle_scope()

    def play_shot(self, result: ShotResult) -> TargetScore | None:
        if self.scene is None:
            return None
        return self.scene.play_shot(result)

    def reset_range(self) -> None:
        if self.scene is not None:
            self.scene.reset()

    def save_screenshot(self, path: Path) -> bool:
        return self.scene is not None and self.scene.save_screenshot(path)

    def shutdown(self) -> None:
        self.render_timer.stop()
        if self.scene is not None:
            self.scene.destroy()
            self.scene = None
        if self.base is not None:
            self.base.destroy()
            self.base = None
