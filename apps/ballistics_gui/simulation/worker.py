"""Qt worker that keeps C simulation and CSV parsing off the GUI thread."""

from __future__ import annotations

from pathlib import Path

from PyQt6.QtCore import QObject, QRunnable, pyqtSignal, pyqtSlot

from .cli_bridge import run_cli_scenario
from .models import ScenarioConfig


class SimulationWorkerSignals(QObject):
    completed = pyqtSignal(int, object)
    failed = pyqtSignal(int, str)
    finished = pyqtSignal(int)


class SimulationWorker(QRunnable):
    def __init__(self, request_id: int, config: ScenarioConfig, cli_path: Path) -> None:
        super().__init__()
        self.request_id = request_id
        self.config = config
        self.cli_path = cli_path
        self.signals = SimulationWorkerSignals()

    @pyqtSlot()
    def run(self) -> None:
        try:
            result = run_cli_scenario(self.config, self.cli_path)
        except Exception as error:  # Worker boundary converts failures to local UI data.
            self.signals.failed.emit(self.request_id, str(error))
        else:
            self.signals.completed.emit(self.request_id, result)
        finally:
            self.signals.finished.emit(self.request_id)
