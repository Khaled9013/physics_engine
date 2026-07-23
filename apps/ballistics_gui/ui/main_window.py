"""Main desktop window and worker orchestration."""

from __future__ import annotations

from datetime import datetime
from pathlib import Path

from PyQt6.QtCore import QThreadPool, Qt, pyqtSlot
from PyQt6.QtWidgets import (
    QFrame,
    QGridLayout,
    QHBoxLayout,
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
        splitter.addWidget(self._build_viewport_placeholder())
        splitter.addWidget(self._build_side_panel())
        splitter.setStretchFactor(0, 1)
        splitter.setSizes([1040, 400])
        self.setCentralWidget(splitter)
        self.statusBar().showMessage("Ready — native range renderer loads in Stage 4")

    def _build_viewport_placeholder(self) -> QWidget:
        frame = QFrame()
        frame.setObjectName("viewportPlaceholder")
        layout = QVBoxLayout(frame)
        title = QLabel("NATIVE 3D RANGE")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title.setStyleSheet("font-size: 28px; font-weight: 800; color: #8fd3ff;")
        note = QLabel("Panda3D viewport integration follows the verified desktop shell.")
        note.setAlignment(Qt.AlignmentFlag.AlignCenter)
        note.setStyleSheet("color: #8194a4;")
        layout.addStretch(1)
        layout.addWidget(title)
        layout.addWidget(note)
        layout.addStretch(1)
        return frame

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
        self.controls.scope_requested.connect(lambda: self._log("Scope requested"))
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
    def fire(self) -> None:
        try:
            config = self.controls.scenario()
        except ValueError as error:
            self._log(f"Invalid scenario: {error}")
            return
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
        self.statusBar().showMessage(f"Shot complete — {summary.stop_reason}")
        self._log(f"Shot {request_id} complete: {summary.sample_count} samples")

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
        for value in self.metrics.values():
            value.setText("—")
        self.controls.set_busy(False)
        self.statusBar().showMessage("Range reset")
        self._log("Range reset; pending results will be ignored")

    def _log(self, message: str) -> None:
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.event_log.appendPlainText(f"[{timestamp}] {message}")

    def closeEvent(self, event) -> None:  # type: ignore[no-untyped-def]
        self._request_id += 1
        self.thread_pool.waitForDone(1500)
        super().closeEvent(event)
