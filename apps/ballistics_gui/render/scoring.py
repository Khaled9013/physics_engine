"""Presentation-only practice-target intersection scoring."""

from __future__ import annotations

from dataclasses import dataclass
import math

from ..simulation.models import TrajectorySample


@dataclass(frozen=True, slots=True)
class TargetScore:
    reached_target: bool
    hit: bool
    lateral_error_m: float | None
    vertical_error_m: float | None
    radial_error_m: float | None


def score_target_plane(
    samples: tuple[TrajectorySample, ...],
    target_distance_m: float,
    target_lateral_m: float = 0.0,
    target_height_m: float = 2.0,
    target_radius_m: float = 0.75,
) -> TargetScore:
    """Linearly interpolate the trajectory at a rendered downrange target plane."""

    if len(samples) < 2 or not all(
        math.isfinite(value)
        for value in (target_distance_m, target_lateral_m, target_height_m, target_radius_m)
    ):
        raise ValueError("target scoring inputs must be finite and include at least two samples")
    if target_distance_m <= 0.0 or target_radius_m <= 0.0:
        raise ValueError("target distance and radius must be positive")

    for previous, current in zip(samples, samples[1:]):
        x0 = previous.position_x_m
        x1 = current.position_x_m
        if (x0 <= target_distance_m <= x1) or (x1 <= target_distance_m <= x0):
            delta = x1 - x0
            if delta == 0.0:
                continue
            alpha = (target_distance_m - x0) / delta
            lateral = previous.position_y_m + alpha * (
                current.position_y_m - previous.position_y_m
            )
            height = previous.position_z_m + alpha * (
                current.position_z_m - previous.position_z_m
            )
            lateral_error = lateral - target_lateral_m
            vertical_error = height - target_height_m
            radial_error = math.hypot(lateral_error, vertical_error)
            return TargetScore(
                reached_target=True,
                hit=radial_error <= target_radius_m,
                lateral_error_m=lateral_error,
                vertical_error_m=vertical_error,
                radial_error_m=radial_error,
            )
    return TargetScore(False, False, None, None, None)
