from __future__ import annotations

from dataclasses import replace
from pathlib import Path
import unittest

from PyQt6.QtCore import QCoreApplication, QEventLoop, QThreadPool, QTimer

from apps.ballistics_gui.simulation.models import ScenarioConfig, ShotResult
from apps.ballistics_gui.simulation.worker import SimulationWorker


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
CLI_PATH = REPOSITORY_ROOT / "build/apps/ballistic_cli/ballistics_cli"


@unittest.skipUnless(CLI_PATH.is_file(), "build the C CLI before running worker test")
class SimulationWorkerTests(unittest.TestCase):
    def test_qthreadpool_worker_returns_real_cli_result(self) -> None:
        application = QCoreApplication.instance() or QCoreApplication([])
        event_loop = QEventLoop()
        thread_pool = QThreadPool()
        completed: list[ShotResult] = []
        failures: list[str] = []
        worker = SimulationWorker(
            17,
            replace(ScenarioConfig(), maximum_time_s=0.05, time_step_s=0.005),
            CLI_PATH,
        )
        worker.signals.completed.connect(lambda request_id, result: completed.append(result))
        worker.signals.failed.connect(lambda request_id, message: failures.append(message))
        worker.signals.finished.connect(lambda request_id: event_loop.quit())
        timeout = QTimer()
        timeout.setSingleShot(True)
        timeout.timeout.connect(event_loop.quit)
        timeout.start(5000)
        thread_pool.start(worker)
        event_loop.exec()
        thread_pool.waitForDone(1000)
        timeout.stop()
        application.processEvents()

        self.assertEqual(failures, [])
        self.assertEqual(len(completed), 1)
        self.assertEqual(completed[0].summary.sample_count, len(completed[0].samples))


if __name__ == "__main__":
    unittest.main()
