"""Immutable values shared by GUI controls, workers, and the renderer."""

from __future__ import annotations

from dataclasses import dataclass
import math

CSV_COLUMNS = (
    "time_s",
    "position_x_m",
    "position_y_m",
    "position_z_m",
    "velocity_x_mps",
    "velocity_y_mps",
    "velocity_z_mps",
    "speed_mps",
    "acceleration_x_mps2",
    "acceleration_y_mps2",
    "acceleration_z_mps2",
)


@dataclass(frozen=True, slots=True)
class ScenarioConfig:
    """One bounded synthetic scenario submitted to the C CLI."""

    integrator: str = "rk4.v1"
    time_step_s: float = 0.002
    maximum_time_s: float = 5.0
    maximum_distance_m: float = 5000.0
    projectile_mass_kg: float = 0.018
    projectile_diameter_m: float = 0.009
    reference_area_m2: float = 6.3617e-5
    launch_speed_mps: float = 310.0
    elevation_deg: float = 4.573921259900861
    azimuth_deg: float = 0.0
    initial_height_m: float = 1.5
    drag_coefficient: float = 0.29
    air_density_kgpm3: float = 1.225
    wind_x_mps: float = 0.0
    wind_y_mps: float = 2.0
    wind_z_mps: float = 0.0
    gravity_mps2: float = 9.80665
    target_distance_m: float = 250.0

    def validate(self) -> None:
        if self.integrator not in {"rk4.v1", "euler.v1"}:
            raise ValueError("integrator must be rk4.v1 or euler.v1")

        ranges = {
            "time_step_s": (0.0001, 0.05),
            "maximum_time_s": (0.05, 30.0),
            "maximum_distance_m": (10.0, 20000.0),
            "projectile_mass_kg": (0.0001, 100.0),
            "projectile_diameter_m": (0.0, 1.0),
            "reference_area_m2": (1.0e-9, 1.0),
            "launch_speed_mps": (0.0, 1500.0),
            "elevation_deg": (-10.0, 85.0),
            "azimuth_deg": (-90.0, 90.0),
            "initial_height_m": (0.01, 1000.0),
            "drag_coefficient": (0.0, 5.0),
            "air_density_kgpm3": (0.0, 10.0),
            "wind_x_mps": (-200.0, 200.0),
            "wind_y_mps": (-200.0, 200.0),
            "wind_z_mps": (-200.0, 200.0),
            "gravity_mps2": (0.0, 50.0),
            "target_distance_m": (25.0, 1000.0),
        }
        for name, (minimum, maximum) in ranges.items():
            value = getattr(self, name)
            if isinstance(value, bool) or not isinstance(value, (int, float)):
                raise ValueError(f"{name} must be numeric")
            if not math.isfinite(value) or value < minimum or value > maximum:
                raise ValueError(f"{name} must be between {minimum:g} and {maximum:g}")

        estimated_steps = math.ceil(self.maximum_time_s / self.time_step_s)
        if estimated_steps > 100_000:
            raise ValueError("scenario exceeds the 100000-step GUI limit")


@dataclass(frozen=True, slots=True)
class TrajectorySample:
    time_s: float
    position_x_m: float
    position_y_m: float
    position_z_m: float
    velocity_x_mps: float
    velocity_y_mps: float
    velocity_z_mps: float
    speed_mps: float
    acceleration_x_mps2: float
    acceleration_y_mps2: float
    acceleration_z_mps2: float


@dataclass(frozen=True, slots=True)
class ShotSummary:
    stop_reason: str
    final_time_s: float
    final_range_x_m: float
    final_range_y_m: float
    final_speed_mps: float
    sample_count: int


@dataclass(frozen=True, slots=True)
class ShotResult:
    scenario: ScenarioConfig
    summary: ShotSummary
    samples: tuple[TrajectorySample, ...]
    csv_text: str
