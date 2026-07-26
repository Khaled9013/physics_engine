"""Main desktop window, renderer integration, and worker orchestration."""

from __future__ import annotations

from datetime import datetime
from pathlib import Path

from PyQt6.QtCore import QThreadPool, Qt, pyqtSlot
from PyQt6.QtGui import QKeySequence, QShortcut
from PyQt6.QtWidgets import (
    QFrame,
    QGridLayout,
    QLabel,
    QMainWindow,
    QPlainTextEdit,
    QSplitter,
    QToolButton,
    QVBoxLayout,
    QWidget,
)

from ..render.range_scene import DEFAULT_SENSITIVITY_SETTING, sensitivity_to_degrees
from ..simulation.models import ShotResult
from ..simulation.worker import SimulationWorker
from ..simulation.zeroing import apply_zero, solve_zero_offset, zero_cache_key
from .controls import ScenarioControls
from .range_widget import RangeWidget
from .settings_dialog import DISPLAY_MODE_FULLSCREEN, SettingsDialog


# Eye height in the rendered scene. The optic's zero is the angle between this line of sight
# and the bore, so the two must agree or the crosshair will not mark the point of impact.
CAMERA_HEIGHT_M = 1.58


class MainWindow(QMainWindow):
    """Native application shell around the Panda3D range and C solver."""

    def __init__(self, cli_path: Path) -> None:
        super().__init__()
        self.cli_path = cli_path
        self.thread_pool = QThreadPool(self)
        self._request_id = 0
        self._workers: dict[int, SimulationWorker] = {}
        self.setWindowTitle("Ballistics Range Lab — Phase 2.2")
        self.resize(1500, 920)
        self.setMinimumSize(1120, 720)

        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.setObjectName("mainSplitter")
        splitter.setChildrenCollapsible(False)
        splitter.setHandleWidth(2)
        self.viewport = RangeWidget()
        self.viewport.fire_requested.connect(self.fire)
        self.viewport.aim_changed.connect(self._aim_changed)
        self.viewport.scope_changed.connect(self._scope_changed)
        self.viewport.renderer_ready.connect(self._renderer_ready)
        self.viewport.renderer_failed.connect(self._renderer_failed)
        self.viewport.settings_requested.connect(self.open_settings)
        splitter.addWidget(self.viewport)
        splitter.addWidget(self._build_side_panel())
        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 0)
        splitter.setSizes([1110, 390])
        self.setCentralWidget(splitter)
        self.statusBar().showMessage("Initializing native GPU range…")

        self.sensitivity_setting = DEFAULT_SENSITIVITY_SETTING
        self._zero_cache: dict[tuple, object] = {}
        self._settings_dialog: SettingsDialog | None = None
        self._windowed_size = (self.width(), self.height())
        # Panda3D only receives the escape key while it holds the pointer. A window-level
        # shortcut covers the rest of the time, so the same key always reaches settings.
        self._settings_shortcut = QShortcut(QKeySequence(Qt.Key.Key_Escape), self)
        self._settings_shortcut.activated.connect(self.open_settings)

    def _build_side_panel(self) -> QWidget:
        panel = QWidget()
        panel.setObjectName("controlPanel")
        panel.setMinimumWidth(355)
        panel.setMaximumWidth(430)
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(18, 18, 18, 12)
        layout.setSpacing(12)

        eyebrow = QLabel("BALLISTICS LAB  /  PHASE 2.2")
        eyebrow.setObjectName("eyebrow")
        layout.addWidget(eyebrow)
        title = QLabel("Virtual Range")
        title.setObjectName("panelTitle")
        layout.addWidget(title)
        subtitle = QLabel("Local simulation · native GPU view · deterministic C core")
        subtitle.setObjectName("panelSubtitle")
        subtitle.setWordWrap(True)
        layout.addWidget(subtitle)
        layout.addWidget(self._build_metrics())

        self.controls = ScenarioControls()
        self.controls.fire_requested.connect(self.fire)
        self.controls.scope_requested.connect(self.viewport.toggle_scope)
        self.controls.reset_requested.connect(self.reset)
        layout.addWidget(self.controls, 1)

        self.log_toggle = QToolButton()
        self.log_toggle.setObjectName("logToggle")
        self.log_toggle.setText("Session log")
        self.log_toggle.setCheckable(True)
        self.log_toggle.setArrowType(Qt.ArrowType.RightArrow)
        self.log_toggle.setToolButtonStyle(
            Qt.ToolButtonStyle.ToolButtonTextBesideIcon
        )
        self.log_toggle.toggled.connect(self._set_log_visible)
        layout.addWidget(self.log_toggle)

        self.event_log = QPlainTextEdit()
        self.event_log.setObjectName("eventLog")
        self.event_log.setReadOnly(True)
        self.event_log.setMaximumBlockCount(100)
        self.event_log.setFixedHeight(92)
        self.event_log.hide()
        layout.addWidget(self.event_log)
        self._log("Desktop shell initialized")
        return panel

    def _build_metrics(self) -> QWidget:
        container = QWidget()
        container.setObjectName("metricsGrid")
        grid = QGridLayout(container)
        grid.setContentsMargins(0, 2, 0, 2)
        grid.setHorizontalSpacing(8)
        grid.setVerticalSpacing(8)
        self.metrics: dict[str, QLabel] = {}
        entries = (
            ("Time", "— s"),
            ("Range", "— m"),
            ("Drift", "— m"),
            ("Speed", "— m/s"),
        )
        for index, (name, initial) in enumerate(entries):
            card = QFrame()
            card.setObjectName("metricCard")
            card_layout = QVBoxLayout(card)
            card_layout.setContentsMargins(10, 8, 10, 9)
            card_layout.setSpacing(2)
            label = QLabel(name.upper())
            label.setObjectName("metricLabel")
            value = QLabel(initial)
            value.setObjectName("metricValue")
            card_layout.addWidget(label)
            card_layout.addWidget(value)
            grid.addWidget(card, index // 2, index % 2)
            self.metrics[name] = value
        return container

    @pyqtSlot()
    def _renderer_ready(self) -> None:
        self.statusBar().showMessage("Range ready — click the viewport to capture aim")
        self._log("Panda3D GPU range initialized")
        self.viewport.set_aim(
            self.controls._fields["elevation_deg"].value(),
            self.controls._fields["azimuth_deg"].value(),
        )

    @pyqtSlot(str)
    def _renderer_failed(self, message: str) -> None:
        self.statusBar().showMessage("Range renderer failed")
        self._log(f"Renderer failed: {message}")

    @pyqtSlot(float, float)
    def _aim_changed(self, elevation_deg: float, azimuth_deg: float) -> None:
        self.controls.set_aim(elevation_deg, azimuth_deg)

    @pyqtSlot(bool)
    def _scope_changed(self, enabled: bool) -> None:
        self._log("Optic raised" if enabled else "Optic lowered")

    def _zero_solution(self, config, zero_distance_m: float):
        """Return the optic's bore-to-sight offset, solving and caching it on first use.

        The offset depends on the load and the environment but not on where the rifle is
        pointed, so it is cached against everything except aim and crosswind.
        """

        key = zero_cache_key(config, zero_distance_m, CAMERA_HEIGHT_M)
        cached = self._zero_cache.get(key)
        if cached is not None:
            return cached
        solution = solve_zero_offset(config, self.cli_path, zero_distance_m, CAMERA_HEIGHT_M)
        self._zero_cache[key] = solution
        if solution.converged:
            self._log(
                f"Optic zeroed at {solution.zero_distance_m:.0f} m: "
                f"{solution.offset_deg:+.4f}° ({solution.offset_mil:+.2f} mil) in "
                f"{solution.iterations} passes"
            )
            self.controls.set_zero_readout(
                f"Zeroed at {solution.zero_distance_m:.0f} m — bore sits "
                f"{solution.offset_mil:+.2f} mil ({solution.offset_moa:+.1f} MOA) above the "
                f"sight line. Beyond that distance the shot drops below the reticle."
            )
        else:
            self._log(f"Optic zeroing failed: {solution.message}")
            self.controls.set_zero_readout(f"Not zeroed — {solution.message}")
        return solution

    @pyqtSlot()
    def fire(self) -> None:
        try:
            config = self.controls.scenario()
        except ValueError as error:
            self._log(f"Invalid scenario: {error}")
            return
        self.viewport.set_aim(config.elevation_deg, config.azimuth_deg)
        # The controls carry the line of sight; the bore is raised by the optic's zero so
        # that the crosshair and the impact agree at the zero distance.
        solution = self._zero_solution(config, self.controls.zero_distance_m())
        config = apply_zero(config, solution)
        self._request_id += 1
        request_id = self._request_id
        worker = SimulationWorker(request_id, config, self.cli_path)
        worker.signals.completed.connect(self._shot_completed)
        worker.signals.failed.connect(self._shot_failed)
        worker.signals.finished.connect(self._worker_finished)
        self._workers[request_id] = worker
        self.controls.set_busy(True)
        self.statusBar().showMessage("Simulating shot in worker thread…")
        self._log(f"Shot {request_id} submitted ({config.integrator})")
        self.thread_pool.start(worker)

    @pyqtSlot(int, object)
    def _shot_completed(self, request_id: int, result: object) -> None:
        if request_id != self._request_id or not isinstance(result, ShotResult):
            return
        summary = result.summary
        self.metrics["Time"].setText(f"{summary.final_time_s:.3f} s")
        self.metrics["Range"].setText(f"{summary.final_range_x_m:.1f} m")
        self.metrics["Drift"].setText(f"{summary.final_range_y_m:.2f} m")
        self.metrics["Speed"].setText(f"{summary.final_speed_mps:.1f} m/s")
        score = self.viewport.play_shot(result)
        if score is None:
            score_text = "renderer unavailable"
        elif score.hit:
            score_text = f"HIT — radial error {score.radial_error_m:.2f} m"
        elif score.reached_target:
            score_text = f"MISS — radial error {score.radial_error_m:.2f} m"
        else:
            score_text = "SHORT — trajectory did not reach target plane"
        self.statusBar().showMessage(f"Shot complete — {score_text}")
        self._log(f"Shot {request_id}: {score_text}; {summary.sample_count} samples")

    @pyqtSlot(int, str)
    def _shot_failed(self, request_id: int, message: str) -> None:
        if request_id != self._request_id:
            return
        self.statusBar().showMessage("Shot failed")
        self._log(f"Shot {request_id} failed: {message}")

    @pyqtSlot(int)
    def _worker_finished(self, request_id: int) -> None:
        self._workers.pop(request_id, None)
        if request_id == self._request_id:
            self.controls.set_busy(False)

    @pyqtSlot()
    def reset(self) -> None:
        self._request_id += 1
        for name, value in self.metrics.items():
            value.setText(f"— {self._metric_unit(name)}")
        self.controls.set_busy(False)
        self.viewport.reset_range()
        self.statusBar().showMessage("Range reset")
        self._log("Range reset; pending results will be ignored")

    @pyqtSlot()
    def open_settings(self) -> None:
        """Raise the escape-key settings panel, or re-focus it if already open."""

        if self._settings_dialog is not None:
            self._settings_dialog.raise_()
            self._settings_dialog.activateWindow()
            return
        self.viewport.release_aim()
        dialog = SettingsDialog(
            sensitivity=self.sensitivity_setting,
            fullscreen=self.isFullScreen(),
            window_size=self._windowed_size,
            sensitivity_preview=sensitivity_to_degrees,
            parent=self,
        )
        dialog.sensitivity_changed.connect(self.apply_sensitivity)
        dialog.display_mode_changed.connect(self.apply_display_mode)
        dialog.window_size_changed.connect(self.apply_window_size)
        self._settings_dialog = dialog
        try:
            dialog.exec()
        finally:
            self._settings_dialog = None
        self.statusBar().showMessage("Settings closed — click the viewport to capture aim")

    @pyqtSlot(float)
    def apply_sensitivity(self, setting: float) -> None:
        self.sensitivity_setting = setting
        self.viewport.set_sensitivity_setting(setting)
        self._log(
            f"Aim speed set to {setting:.0f} "
            f"({sensitivity_to_degrees(setting):.4f} deg/count)"
        )

    @pyqtSlot(str)
    def apply_display_mode(self, mode: str) -> None:
        if mode == DISPLAY_MODE_FULLSCREEN:
            if not self.isFullScreen():
                self._windowed_size = (self.width(), self.height())
            self.showFullScreen()
        else:
            self.showNormal()
            self.resize(*self._windowed_size)
        self._log(f"Display mode set to {mode.lower()}")

    @pyqtSlot(int, int)
    def apply_window_size(self, width: int, height: int) -> None:
        self._windowed_size = (width, height)
        if self.isFullScreen():
            return
        # A preset may exceed the current minimum on a small display; Qt clamps it, and the
        # stored preference is kept so the choice survives a later mode change.
        self.showNormal()
        self.resize(width, height)
        self._log(f"Window size set to {width} x {height}")

    @pyqtSlot(bool)
    def _set_log_visible(self, visible: bool) -> None:
        self.event_log.setVisible(visible)
        self.log_toggle.setArrowType(
            Qt.ArrowType.DownArrow if visible else Qt.ArrowType.RightArrow
        )

    @staticmethod
    def _metric_unit(name: str) -> str:
        return {"Time": "s", "Range": "m", "Drift": "m", "Speed": "m/s"}[name]

    def save_viewport_screenshot(self, path: Path) -> bool:
        return self.viewport.save_screenshot(path)

    def save_window_screenshot(self, path: Path) -> bool:
        screen = self.screen()
        if screen is None:
            return False
        return screen.grabWindow(int(self.winId())).save(str(path))

    def _log(self, message: str) -> None:
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.event_log.appendPlainText(f"[{timestamp}] {message}")

    def closeEvent(self, event) -> None:  # type: ignore[no-untyped-def]
        self._request_id += 1
        self.thread_pool.waitForDone(1500)
        self.viewport.shutdown()
        super().closeEvent(event)
