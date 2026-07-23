"""The single mapping between ballistics and Panda3D coordinates."""

from __future__ import annotations

from ..simulation.models import TrajectorySample


def ballistics_to_panda(x_downrange: float, y_right: float, z_up: float) -> tuple[float, float, float]:
    """Map +x downrange/+y right/+z up to Panda +y forward/+x right/+z up."""

    return (y_right, x_downrange, z_up)


def sample_to_panda(sample: TrajectorySample) -> tuple[float, float, float]:
    return ballistics_to_panda(sample.position_x_m, sample.position_y_m, sample.position_z_m)
