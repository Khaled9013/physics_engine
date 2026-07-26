"""Optic zeroing.

The reticle marks the line of sight. A projectile leaves the bore along the bore axis and
then falls, so if the bore and the line of sight were parallel every shot would land below
the crosshair by the full drop — 1.3 m at 150 m with the default load. A real optic is
zeroed: the bore is angled slightly above the line of sight so the two cross at a chosen
distance.

This module finds that angle. It does not integrate anything itself; it drives the same C
solver the application always uses and searches for the launch elevation whose trajectory
crosses the sight line at the zero distance. Python never substitutes its own physics.
"""

from __future__ import annotations

from dataclasses import dataclass, replace
import math
from pathlib import Path

from .cli_bridge import run_cli_scenario
from .models import ScenarioConfig
from ..render.scoring import score_target_plane


DEFAULT_ZERO_DISTANCE_M = 150.0
DEFAULT_TOLERANCE_M = 0.005
DEFAULT_MAX_ITERATIONS = 8
PROBE_TIME_STEP_S = 0.001

# Elevation is bounded by the scenario contract; the search must stay inside it.
ELEVATION_LIMITS_DEG = (-10.0, 85.0)


@dataclass(frozen=True)
class ZeroSolution:
    """The bore-to-sight angle that puts point of impact on point of aim."""

    offset_deg: float
    zero_distance_m: float
    iterations: int
    residual_m: float
    converged: bool
    message: str = ""

    @property
    def offset_mil(self) -> float:
        """Offset in milliradians, the usual unit on a target optic's elevation turret."""

        return math.radians(self.offset_deg) * 1000.0

    @property
    def offset_moa(self) -> float:
        return self.offset_deg * 60.0


def zero_cache_key(config: ScenarioConfig, zero_distance_m: float, sight_height_m: float):
    """Return the values a zero depends on.

    Aim is deliberately excluded: the offset is a fixed mechanical relationship between bore
    and sight, so it survives the shooter moving the rifle.
    """

    return (
        config.integrator,
        config.time_step_s,
        config.maximum_time_s,
        config.maximum_distance_m,
        config.projectile_mass_kg,
        config.reference_area_m2,
        config.drag_coefficient,
        config.launch_speed_mps,
        config.initial_height_m,
        config.air_density_kgpm3,
        config.wind_x_mps,
        config.wind_z_mps,
        config.gravity_mps2,
        zero_distance_m,
        sight_height_m,
    )


def _impact_height_m(
    config: ScenarioConfig, cli_path: Path, elevation_deg: float, distance_m: float
) -> float | None:
    """Return the trajectory's height where it crosses ``distance_m``, or None if short."""

    probe = replace(
        config,
        elevation_deg=elevation_deg,
        azimuth_deg=0.0,
        # A zero is established in still air; crosswind is a condition to hold off for, not
        # something to build into the mechanical offset.
        wind_y_mps=0.0,
        # Solving needs several runs, and at the finest allowed step that is a visible stall
        # before the first shot. RK4 has fully converged by 1 ms here — the resulting angle
        # is identical to five decimal places — so probes never integrate finer than that.
        time_step_s=max(config.time_step_s, PROBE_TIME_STEP_S),
    )
    result = run_cli_scenario(probe, cli_path)
    score = score_target_plane(result.samples, distance_m, 0.0, 0.0)
    if not score.reached_target or score.vertical_error_m is None:
        return None
    return score.vertical_error_m


def solve_zero_offset(
    config: ScenarioConfig,
    cli_path: Path,
    zero_distance_m: float = DEFAULT_ZERO_DISTANCE_M,
    sight_height_m: float = 1.58,
    *,
    tolerance_m: float = DEFAULT_TOLERANCE_M,
    max_iterations: int = DEFAULT_MAX_ITERATIONS,
) -> ZeroSolution:
    """Find the launch elevation that puts the trajectory on the sight line at one distance.

    The sight line is taken horizontal at ``sight_height_m``, so the result is the angle
    between bore and sight. Applying it as a constant offset to any aim is the same
    approximation a real fixed-mount optic makes, and is accurate to a few centimetres over
    the elevations this application allows.

    Convergence is by direct correction rather than a general root finder: raising the launch
    angle by a small angle d raises the impact by approximately ``distance * d``, so each
    step is a near-exact Newton step and two or three passes are typical.
    """

    if zero_distance_m <= 0.0:
        return ZeroSolution(0.0, zero_distance_m, 0, 0.0, False, "zero distance must be positive")

    elevation, height, probes = _first_reaching_elevation(config, cli_path, zero_distance_m)
    if height is None:
        return ZeroSolution(
            0.0,
            zero_distance_m,
            probes,
            float("inf"),
            False,
            f"trajectory cannot reach {zero_distance_m:.0f} m at any supported elevation",
        )

    residual = sight_height_m - height
    for iteration in range(1, max_iterations + 1):
        if abs(residual) <= tolerance_m:
            return ZeroSolution(elevation, zero_distance_m, probes + iteration - 1, residual, True)
        proposed = elevation + math.degrees(math.atan2(residual, zero_distance_m))
        proposed = min(proposed, ELEVATION_LIMITS_DEG[1])
        if proposed <= ELEVATION_LIMITS_DEG[0]:
            return ZeroSolution(
                0.0,
                zero_distance_m,
                probes + iteration,
                residual,
                False,
                "required zero angle is below the supported elevation range",
            )
        height = _impact_height_m(config, cli_path, proposed, zero_distance_m)
        if height is None:
            # A correction can undershoot back past the maximum range. Split the step rather
            # than abandoning a zero that is known to exist between here and the last probe.
            proposed = (elevation + proposed) * 0.5
            height = _impact_height_m(config, cli_path, proposed, zero_distance_m)
            if height is None:
                return ZeroSolution(
                    elevation,
                    zero_distance_m,
                    probes + iteration,
                    residual,
                    False,
                    f"trajectory became short of {zero_distance_m:.0f} m while converging",
                )
        elevation = proposed
        residual = sight_height_m - height

    return ZeroSolution(
        elevation,
        zero_distance_m,
        probes + max_iterations,
        residual,
        False,
        f"zero did not settle within {max_iterations} passes",
    )


def _first_reaching_elevation(
    config: ScenarioConfig, cli_path: Path, zero_distance_m: float
) -> tuple[float, float | None, int]:
    """Find the lowest trial elevation whose trajectory actually reaches the zero distance.

    Starting the search at level is wrong for anything but a short zero: a projectile fired
    horizontally from shoulder height falls to the ground long before a distant zero, so the
    first probe returns nothing even though a perfectly good zero exists higher up. The angle
    is escalated until the shot carries far enough.
    """

    elevation = 0.0
    for probe in range(1, 12):
        height = _impact_height_m(config, cli_path, elevation, zero_distance_m)
        if height is not None:
            return elevation, height, probe
        elevation = elevation * 2.0 + 0.25
        if elevation > ELEVATION_LIMITS_DEG[1]:
            return elevation, None, probe
    return elevation, None, 11


def apply_zero(config: ScenarioConfig, solution: ZeroSolution) -> ScenarioConfig:
    """Return the launch scenario for an aim, with the bore raised by the zero offset."""

    if not solution.converged:
        return config
    elevation = config.elevation_deg + solution.offset_deg
    elevation = max(ELEVATION_LIMITS_DEG[0], min(ELEVATION_LIMITS_DEG[1], elevation))
    return replace(config, elevation_deg=elevation)
