from __future__ import annotations

from dataclasses import replace
from pathlib import Path
import tempfile
import unittest

from apps.ballistics_gui.simulation.cli_bridge import (
    SimulationError,
    build_cli_arguments,
    parse_cli_output,
    run_cli_scenario,
)
from apps.ballistics_gui.simulation.models import CSV_COLUMNS, ScenarioConfig


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
CLI_PATH = REPOSITORY_ROOT / "build/apps/ballistic_cli/ballistics_cli"


class ScenarioConfigTests(unittest.TestCase):
    def test_defaults_are_valid(self) -> None:
        ScenarioConfig().validate()

    def test_invalid_integrator_is_rejected(self) -> None:
        with self.assertRaisesRegex(ValueError, "integrator"):
            replace(ScenarioConfig(), integrator="unknown.v1").validate()

    def test_non_finite_and_excessive_work_are_rejected(self) -> None:
        with self.assertRaisesRegex(ValueError, "launch_speed_mps"):
            replace(ScenarioConfig(), launch_speed_mps=float("nan")).validate()
        with self.assertRaisesRegex(ValueError, "100000-step"):
            replace(ScenarioConfig(), time_step_s=0.0001, maximum_time_s=30.0).validate()

    def test_cli_arguments_are_fixed_and_include_scenario(self) -> None:
        config = replace(ScenarioConfig(), azimuth_deg=-3.5, wind_y_mps=-7.0)
        arguments = build_cli_arguments(config, Path("/tmp/gui-output.csv"))
        self.assertIn("--azimuth-deg", arguments)
        self.assertEqual(arguments[arguments.index("--azimuth-deg") + 1], "-3.5")
        self.assertEqual(arguments[arguments.index("--wind-y") + 1], "-7")
        self.assertEqual(arguments[arguments.index("--output") + 1], "/tmp/gui-output.csv")


class CliBridgeTests(unittest.TestCase):
    def test_bad_csv_header_is_rejected(self) -> None:
        with self.assertRaisesRegex(SimulationError, "header"):
            parse_cli_output(ScenarioConfig(), "bad,header\n1,2\n", "")

    @unittest.skipUnless(CLI_PATH.is_file(), "build the C CLI before running integration test")
    def test_real_cli_result_and_csv_are_deterministic(self) -> None:
        config = replace(ScenarioConfig(), maximum_time_s=0.05, time_step_s=0.005)
        first = run_cli_scenario(config, CLI_PATH)
        second = run_cli_scenario(config, CLI_PATH)
        self.assertEqual(first.csv_text, second.csv_text)
        self.assertEqual(first.summary.sample_count, len(first.samples))
        self.assertEqual(tuple(first.csv_text.splitlines()[0].split(",")), CSV_COLUMNS)
        self.assertGreater(len(first.samples), 1)

    def test_missing_executable_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / "missing-cli"
            with self.assertRaisesRegex(SimulationError, "not executable"):
                run_cli_scenario(ScenarioConfig(), missing)


if __name__ == "__main__":
    unittest.main()
