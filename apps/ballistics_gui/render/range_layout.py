"""Declarative range layout.

This module holds the deterministic placement of every piece of range dressing as plain
data. It imports nothing from Panda3D so the layout can be inspected and tested headlessly,
and so `environment.py` stays a builder rather than a mixture of builder and content.

All positions are metres in the render frame: ``+x`` right of the shooter, ``+y`` downrange,
``+z`` up. The shooter stands at the origin.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class TerrainBand:
    """One ground mesh covering a downrange interval."""

    name: str
    x_range: tuple[float, float]
    y_range: tuple[float, float]
    divisions: tuple[int, int]
    tile_size_m: float
    amplitude: float
    y_bias: float
    # Multiplies the committed diffuse map. The vendored grass scans are dark and strongly
    # brown while the gravel scan is a pale limestone chipping, so without correction the
    # lane blows out against a near-black field.
    tint: tuple[float, float, float, float]


@dataclass(frozen=True)
class BoxPlacement:
    name: str
    position: tuple[float, float, float]
    scale: tuple[float, float, float]
    color: tuple[float, float, float, float]


@dataclass(frozen=True)
class RidgePlacement:
    """A distant landform, tinted toward the haze to sit behind the atmosphere."""

    position: tuple[float, float, float]
    scale: tuple[float, float, float]
    color: tuple[float, float, float, float]


@dataclass(frozen=True)
class TreePlacement:
    lateral: float
    distance: float
    scale: float
    lean_deg: float


@dataclass(frozen=True)
class TargetBayPlacement:
    lateral: float
    distance: float


# The near band carries fine grass detail where the camera can resolve it. The far band
# switches to a coarser rock-and-grass texture at a much larger tile size, which reads as
# terrain variation instead of a repeating stamp. The overlap hides the seam.
NEAR_FIELD = TerrainBand(
    name="near-field-ground",
    x_range=(-430.0, 430.0),
    y_range=(-70.0, 210.0),
    divisions=(86, 66),
    tile_size_m=5.5,
    amplitude=1.0,
    y_bias=1.0,
    tint=(1.45, 1.48, 1.15, 1.0),
)

FAR_FIELD = TerrainBand(
    name="far-field-ground",
    x_range=(-1500.0, 1500.0),
    y_range=(185.0, 3200.0),
    divisions=(64, 72),
    tile_size_m=44.0,
    amplitude=2.6,
    y_bias=1.9,
    tint=(1.10, 1.16, 1.00, 1.0),
)

LANE_HALF_WIDTH_M = 6.4
LANE_START_M = -9.0
LANE_END_M = 1060.0
LANE_TILE_SIZE_M = 3.1
LANE_TINT = (0.50, 0.49, 0.47, 1.0)
LANE_SURFACE_Z = 0.02


def distance_markers() -> tuple[BoxPlacement, ...]:
    """Return lane distance boards every 50 m, emphasised on each 100 m."""

    markers = []
    for distance in range(50, 1001, 50):
        hundred = distance % 100 == 0
        markers.append(
            BoxPlacement(
                name="distance-marker-post",
                position=(-LANE_HALF_WIDTH_M - 0.45, float(distance), 0.34),
                scale=(0.05, 0.05, 0.34),
                color=(0.30, 0.29, 0.26, 1.0),
            )
        )
        markers.append(
            BoxPlacement(
                name="distance-marker-board",
                position=(-LANE_HALF_WIDTH_M - 0.45, float(distance), 0.62),
                scale=(0.30 if hundred else 0.21, 0.03, 0.20 if hundred else 0.14),
                color=(0.86, 0.83, 0.74, 1.0) if hundred else (0.55, 0.56, 0.52, 1.0),
            )
        )
    return tuple(markers)


FIRING_LINE: tuple[BoxPlacement, ...] = (
    BoxPlacement(
        name="firing-pad",
        position=(0.0, -1.8, 0.045),
        scale=(5.2, 4.6, 0.045),
        color=(0.44, 0.43, 0.40, 1.0),
    ),
    BoxPlacement(
        name="firing-pad-lip",
        position=(0.0, 2.85, 0.10),
        scale=(5.25, 0.12, 0.10),
        color=(0.33, 0.32, 0.30, 1.0),
    ),
    BoxPlacement(
        name="shooting-bench-top",
        position=(-2.35, 1.45, 0.92),
        scale=(0.85, 0.42, 0.04),
        color=(0.34, 0.26, 0.17, 1.0),
    ),
    BoxPlacement(
        name="shooting-bench-leg-left",
        position=(-3.05, 1.45, 0.46),
        scale=(0.045, 0.045, 0.46),
        color=(0.20, 0.21, 0.21, 1.0),
    ),
    BoxPlacement(
        name="shooting-bench-leg-right",
        position=(-1.65, 1.45, 0.46),
        scale=(0.045, 0.045, 0.46),
        color=(0.20, 0.21, 0.21, 1.0),
    ),
    BoxPlacement(
        name="equipment-case-left",
        position=(-2.9, 2.1, 0.22),
        scale=(0.52, 0.30, 0.22),
        color=(0.17, 0.20, 0.17, 1.0),
    ),
    BoxPlacement(
        name="equipment-case-right",
        position=(3.1, 1.6, 0.19),
        scale=(0.44, 0.26, 0.19),
        color=(0.21, 0.19, 0.15, 1.0),
    ),
    BoxPlacement(
        name="bay-post-left",
        position=(-5.35, 0.9, 1.35),
        scale=(0.09, 0.09, 1.35),
        color=(0.24, 0.22, 0.19, 1.0),
    ),
    BoxPlacement(
        name="bay-post-right",
        position=(5.35, 0.9, 1.35),
        scale=(0.09, 0.09, 1.35),
        color=(0.24, 0.22, 0.19, 1.0),
    ),
    BoxPlacement(
        name="bay-beam",
        position=(0.0, 0.9, 2.62),
        scale=(5.4, 0.08, 0.09),
        color=(0.22, 0.20, 0.17, 1.0),
    ),
)


# Side berms frame the lane. They sit far enough out to stay clear of the flat lane corridor
# and low enough that they never occlude a target.
BERMS: tuple[RidgePlacement, ...] = (
    RidgePlacement((-38.0, 420.0, -2.6), (22.0, 470.0, 5.2), (0.36, 0.30, 0.19, 1.0)),
    RidgePlacement((38.0, 420.0, -2.6), (22.0, 470.0, 5.2), (0.36, 0.30, 0.19, 1.0)),
    RidgePlacement((0.0, 1090.0, -3.0), (62.0, 30.0, 11.0), (0.34, 0.29, 0.19, 1.0)),
)


# Distant landforms are pushed far beyond the lane so the exponential haze desaturates them
# into genuine aerial perspective instead of standing as hard grey walls behind the target.
RIDGES: tuple[RidgePlacement, ...] = (
    RidgePlacement((-2600.0, 5200.0, -220.0), (1500.0, 900.0, 520.0), (0.22, 0.25, 0.27, 1.0)),
    RidgePlacement((-400.0, 6400.0, -260.0), (1900.0, 1100.0, 640.0), (0.24, 0.27, 0.29, 1.0)),
    RidgePlacement((2300.0, 5600.0, -240.0), (1400.0, 850.0, 560.0), (0.23, 0.26, 0.28, 1.0)),
    RidgePlacement((900.0, 3900.0, -180.0), (1100.0, 620.0, 380.0), (0.20, 0.23, 0.25, 1.0)),
    RidgePlacement((-1500.0, 3400.0, -160.0), (900.0, 520.0, 320.0), (0.19, 0.22, 0.24, 1.0)),
)


TREES: tuple[TreePlacement, ...] = (
    TreePlacement(-21.0, 38.0, 1.15, 3.0),
    TreePlacement(26.0, 61.0, 1.32, -2.0),
    TreePlacement(-30.0, 96.0, 1.24, 1.5),
    TreePlacement(35.0, 138.0, 1.44, -3.5),
    TreePlacement(-24.0, 152.0, 0.96, 2.5),
    TreePlacement(-44.0, 214.0, 1.52, -1.0),
    TreePlacement(46.0, 296.0, 1.38, 2.0),
    TreePlacement(-52.0, 392.0, 1.61, -2.5),
    TreePlacement(55.0, 508.0, 1.47, 1.0),
    TreePlacement(-63.0, 640.0, 1.55, -1.5),
    TreePlacement(68.0, 780.0, 1.42, 2.0),
    TreePlacement(-78.0, 910.0, 1.66, -3.0),
)


TARGET_BAYS: tuple[TargetBayPlacement, ...] = (
    TargetBayPlacement(-4.3, 100.0),
    TargetBayPlacement(4.5, 250.0),
    TargetBayPlacement(-4.4, 500.0),
    TargetBayPlacement(4.6, 750.0),
)


# Near-field clutter. A ground plane that meets the camera with no geometry on it is the
# clearest tell that a scene is synthetic, so tufts are scattered around the firing line.
def grass_tufts() -> tuple[tuple[float, float, float, float], ...]:
    """Return ``(x, y, scale, heading)`` for deterministic near-field grass tufts."""

    tufts = []
    for index in range(150):
        # A low-discrepancy pair keeps the scatter even without a random generator, so the
        # arrangement is identical on every launch.
        u = (index * 0.7548776662) % 1.0
        v = (index * 0.5698402909) % 1.0
        x = (u - 0.5) * 74.0
        y = -12.0 + v * 96.0
        if abs(x) < 7.2 and -8.0 < y < 6.0:
            continue
        scale = 0.55 + ((index * 0.3819660113) % 1.0) * 0.75
        heading = ((index * 0.6180339887) % 1.0) * 360.0
        tufts.append((x, y, scale, heading))
    return tuple(tufts)
