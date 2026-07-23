"""Validated subprocess bridge to the authoritative C simulation CLI."""

from __future__ import annotations

import csv
import io
import math
import os
from pathlib import Path
import re
import subprocess
import tempfile

from .models import CSV_COLUMNS, ScenarioConfig, ShotResult, ShotSummary, TrajectorySample

_SUMMARY_PATTERN = re.compile(
    r"^stop=(?P<reason>.+?) time=(?P<time>[-+0-9.eE]+) s "
    r"range=\((?P<x>[-+0-9.eE]+), (?P<y>[-+0-9.eE]+)\) m "
    r"speed=(?P<speed>[-+0-9.eE]+) m/s samples=(?P<count>[0-9]+) "
    r"output=.+$"
)


class SimulationError(RuntimeError):
    """A safe application-layer simulation failure."""


def _number(value: float) -> str:
    return format(value, ".17g")


def build_cli_arguments(config: ScenarioConfig, output_path: Path) -> list[str]:
    """Build the fixed, shell-free CLI argument list for one scenario."""

    config.validate()
    return [
        "--integrator", config.integrator,
        "--time-step", _number(config.time_step_s),
        "--max-time", _number(config.maximum_time_s),
        "--max-distance", _number(config.maximum_distance_m),
        "--mass", _number(config.projectile_mass_kg),
        "--diameter", _number(config.projectile_diameter_m),
        "--reference-area", _number(config.reference_area_m2),
        "--launch-speed", _number(config.launch_speed_mps),
        "--elevation-deg", _number(config.elevation_deg),
        "--azimuth-deg", _number(config.azimuth_deg),
        "--initial-height", _number(config.initial_height_m),
        "--drag-coefficient", _number(config.drag_coefficient),
        "--air-density", _number(config.air_density_kgpm3),
        "--wind-x", _number(config.wind_x_mps),
        "--wind-y", _number(config.wind_y_mps),
        "--wind-z", _number(config.wind_z_mps),
        "--gravity", _number(config.gravity_mps2),
        "--output", str(output_path),
        "--debug-level", "warning",
    ]


def parse_cli_output(config: ScenarioConfig, csv_text: str, stdout: str) -> ShotResult:
    """Parse and validate deterministic CSV plus the CLI summary line."""

    reader = csv.reader(io.StringIO(csv_text, newline=""))
    try:
        header = tuple(next(reader))
    except StopIteration as error:
        raise SimulationError("simulation produced an empty CSV") from error
    if header != CSV_COLUMNS:
        raise SimulationError("simulation produced an unexpected CSV header")

    samples: list[TrajectorySample] = []
    for row_number, row in enumerate(reader, start=2):
        if len(row) != len(CSV_COLUMNS):
            raise SimulationError(f"CSV row {row_number} has an unexpected column count")
        try:
            values = tuple(float(value) for value in row)
        except ValueError as error:
            raise SimulationError(f"CSV row {row_number} contains invalid numeric data") from error
        if not all(math.isfinite(value) for value in values):
            raise SimulationError(f"CSV row {row_number} contains non-finite data")
        samples.append(TrajectorySample(*values))
    if not samples:
        raise SimulationError("simulation produced no trajectory samples")

    summary_line = stdout.strip().splitlines()[-1] if stdout.strip() else ""
    match = _SUMMARY_PATTERN.match(summary_line)
    if match is None:
        raise SimulationError("simulation produced an unexpected summary")
    summary = ShotSummary(
        stop_reason=match.group("reason"),
        final_time_s=float(match.group("time")),
        final_range_x_m=float(match.group("x")),
        final_range_y_m=float(match.group("y")),
        final_speed_mps=float(match.group("speed")),
        sample_count=int(match.group("count")),
    )
    if summary.sample_count != len(samples):
        raise SimulationError("CSV sample count does not match the simulation summary")
    return ShotResult(config, summary, tuple(samples), csv_text)


def run_cli_scenario(
    config: ScenarioConfig,
    cli_path: Path,
    timeout_seconds: float = 20.0,
) -> ShotResult:
    """Run one scenario in an isolated temporary directory."""

    config.validate()
    executable = Path(cli_path).resolve()
    if not executable.is_file() or not os.access(executable, os.X_OK):
        raise SimulationError(f"simulation CLI is not executable: {executable}")
    if not math.isfinite(timeout_seconds) or timeout_seconds <= 0.0:
        raise ValueError("timeout_seconds must be positive and finite")

    environment = os.environ.copy()
    environment["LC_ALL"] = "C"
    environment["LANG"] = "C"
    with tempfile.TemporaryDirectory(prefix="ballistics-gui-") as temporary_directory:
        output_path = Path(temporary_directory) / "trajectory.csv"
        command = [str(executable), *build_cli_arguments(config, output_path)]
        try:
            completed = subprocess.run(
                command,
                check=False,
                capture_output=True,
                text=True,
                timeout=timeout_seconds,
                env=environment,
            )
        except subprocess.TimeoutExpired as error:
            raise SimulationError("simulation timed out") from error
        except OSError as error:
            raise SimulationError(f"unable to start simulation: {error.strerror}") from error
        if completed.returncode != 0:
            detail = completed.stderr.strip().splitlines()[-1] if completed.stderr.strip() else ""
            message = "simulation CLI rejected the scenario"
            if detail:
                message = f"{message}: {detail}"
            raise SimulationError(message)
        try:
            csv_text = output_path.read_bytes().decode("ascii")
        except (OSError, UnicodeDecodeError) as error:
            raise SimulationError("unable to read simulation CSV output") from error
    return parse_cli_output(config, csv_text, completed.stdout)
