"""Thread-safe simulation bridge for the native GUI."""

from .cli_bridge import SimulationError, run_cli_scenario
from .models import ScenarioConfig, ShotResult, ShotSummary, TrajectorySample

__all__ = [
    "ScenarioConfig",
    "ShotResult",
    "ShotSummary",
    "SimulationError",
    "TrajectorySample",
    "run_cli_scenario",
]
