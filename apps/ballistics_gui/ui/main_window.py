"""Main desktop window, renderer integration, and worker orchestration."""

from __future__ import annotations

from datetime import datetime
from pathlib import Path

from PyQt6.QtCore import QThreadPool, Qt, pyqtSlot
from PyQt6.QtWidgets import (
    QFrame,
    QGridLayout,
    QLabel,
    QMainWindow,
    QPlainTextEdit,
    QSplitter,
    QVBoxLayout,
    QWidget,
)

from ..simulation.models import ShotResult
from ..simulation.worker import SimulationWorker
from .controls import ScenarioControls
from .range_widget import RangeWidget


class MainWindow(QMainWindow):
    def __init__(self, cli_path: Path) -> None:
        super().__init__()
        self.cli_path = cli_path
        self.thread_pool = QThreadPool(self)
        self._request_id = 0
        self._workers: dict[int, SimulationWorker] = {}
        self.setWindowTitle("Ballistics Range Lab — Phase Two")
        self.resize(1440, 900)
        self.setMinimumSize(1100, 700)

        splitter = QSplitter(Qt.Orientation.Horizontal)
        self.viewport = RangeWidget()
        self.viewport.fire_requested.connect(self.fire)
        self.viewport.aim_changed.connect(self._aim_changed)
        self.viewport.scope_changed.connect(self._scope_changed)
        self.viewport.renderer_ready.connect(self._renderer_ready)
        self.viewport.renderer_failed.connect(self._renderer_failed)
        splitter.addWidget(self.viewport)
        splitter.addWidget(self._build_side_panel())
        splitter.setStretchFactor(0, 1)
        splitter.setSizes([1040, 400])
        self.setCentralWidget(splitter)
        self.statusBar().showMessage("Initializing native GPU range…")

    def _build_side_panel(self) -> QWidget:
        panel = QWidget()
        panel.setMinimumWidth(350)
        panel.setMaximumWidth(470)
        layout = QVBoxLayout(panel)
        heading = QLabel("RANGE CONTROL")
        heading.setStyleSheet("font-size: 20px; font-weight: 800; color: #8fd3ff;")
        layout.addWidget(heading)
        self.metrics: dict[str, QLabel] = {}
        metrics = QFrame()
        grid = QGridLayout(metrics)
        for index, (name, initial) in enumerate(
            [("Time", "— s"), ("Range", "— m"), ("Drift", "— m"), ("Speed", "— m/s")]
        ):
            label = QLabel(name.upper())
            value = QLabel(initial)
            value.setObjectName("metricValue")
            grid.addWidget(label, index // 2 * 2, index % 2)
            grid.addWidget(value, index // 2 * 2 + 1, index % 2)
            self.metrics[name] = value
        layout.addWidget(metrics)

        self.controls = ScenarioControls()
        self.controls.fire_requested.connect(self.fire)
        self.controls.scope_requested.connect(self.viewport.toggle_scope)
        self.controls.reset_requested.connect(self.reset)
        layout.addWidget(self.controls, 1)
        self.event_log = QPlainTextEdit()
        self.event_log.setReadOnly(True)
        self.event_log.setMaximumBlockCount(100)
        self.event_log.setFixedHeight(120)
        layout.addWidget(self.event_log)
        self._log("Desktop shell initialized")
        return panel

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
        self._log("Scope raised" if enabled else "Scope lowered")

    @pyqtSlot()
    def fire(self) -> None:
        try:
            config = self.controls.scenario()
        except ValueError as error:
            self._log(f"Invalid scenario: {error}")
            return
        self.viewport.set_aim(config.elevation_deg, config.azimuth_deg)
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
