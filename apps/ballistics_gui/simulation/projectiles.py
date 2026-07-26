"""Committed projectile presets and their validation.

Presets are data, not code, so they live in `data/projectiles.json` and are loaded from a
path resolved relative to this package rather than to the working directory. Nothing here
touches the network; the file is committed alongside the application.

Reference area is deliberately not stored in the file. It is derived from diameter, which is
the fix for a defect where the two were independent fields that could silently disagree
while only one of them reached the solver.
"""

from __future__ import annotations

from dataclasses import dataclass
import json
import math
from pathlib import Path


PRESET_PATH = Path(__file__).resolve().parents[1] / "data" / "projectiles.json"
SUPPORTED_SCHEMA_VERSION = 1

# Guard rails matching the solver's accepted domain. A preset outside these would be
# rejected at simulation time, so it is rejected at load time instead, with the offending
# preset named.
MASS_RANGE_KG = (1.0e-6, 100.0)
DIAMETER_RANGE_M = (1.0e-4, 1.0)
DRAG_COEFFICIENT_RANGE = (0.0, 5.0)
MUZZLE_SPEED_RANGE_MPS = (0.0, 1500.0)


def circular_reference_area_m2(diameter_m: float) -> float:
    """Return the frontal area of a circular cross-section."""

    return math.pi * diameter_m * diameter_m / 4.0


@dataclass(frozen=True)
class ProjectilePreset:
    """One selectable projectile.

    ``drag_coefficient`` is a single representative value. The solver applies a constant
    coefficient and does not model the Mach dependence of real drag, so the value is chosen
    to reproduce ``reference_speed_mps`` at ``reference_distance_m`` and is progressively
    less accurate away from that band.
    """

    identifier: str
    name: str
    category: str
    mass_kg: float
    diameter_m: float
    drag_coefficient: float
    muzzle_speed_mps: float
    reference_distance_m: float
    reference_speed_mps: float
    description: str

    @property
    def reference_area_m2(self) -> float:
        return circular_reference_area_m2(self.diameter_m)

    @property
    def sectional_density_kgpm2(self) -> float:
        """Mass per unit frontal cross-section, the usual measure of how well a projectile
        carries velocity. Real small-arms bullets sit near 100-300 kg/m^2."""

        return self.mass_kg / (self.diameter_m * self.diameter_m)

    def expected_speed_at_reference(self, air_density_kgpm3: float = 1.225) -> float:
        """Return the speed this preset's coefficient predicts at its reference distance.

        Drag alone over a flat path gives ``v(x) = v0 * exp(-k x)`` with
        ``k = rho*Cd*A/(2m)``. Comparing this with ``reference_speed_mps`` shows whether a
        coefficient still matches the published figure it was derived from.
        """

        k = air_density_kgpm3 * self.drag_coefficient * self.reference_area_m2 / (2.0 * self.mass_kg)
        return self.muzzle_speed_mps * math.exp(-k * self.reference_distance_m)


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def _read_number(entry: dict, key: str, identifier: str, bounds: tuple[float, float]) -> float:
    _require(key in entry, f"projectile preset {identifier!r} is missing {key!r}")
    value = entry[key]
    _require(
        isinstance(value, (int, float)) and not isinstance(value, bool),
        f"projectile preset {identifier!r} has a non-numeric {key!r}",
    )
    value = float(value)
    _require(
        math.isfinite(value) and bounds[0] <= value <= bounds[1],
        f"projectile preset {identifier!r} has {key}={value} outside {bounds}",
    )
    return value


def _read_text(entry: dict, key: str, identifier: str) -> str:
    _require(key in entry, f"projectile preset {identifier!r} is missing {key!r}")
    value = entry[key]
    _require(
        isinstance(value, str) and value.strip() != "",
        f"projectile preset {identifier!r} has an empty {key!r}",
    )
    return value


def parse_projectile_presets(document: dict) -> tuple[ProjectilePreset, ...]:
    """Validate a decoded preset document and return its presets in file order."""

    _require(isinstance(document, dict), "projectile preset file must contain an object")
    version = document.get("schema_version")
    _require(
        version == SUPPORTED_SCHEMA_VERSION,
        f"unsupported projectile preset schema {version!r}; expected {SUPPORTED_SCHEMA_VERSION}",
    )
    raw_presets = document.get("presets")
    _require(
        isinstance(raw_presets, list) and raw_presets,
        "projectile preset file declares no presets",
    )

    presets: list[ProjectilePreset] = []
    seen: set[str] = set()
    for index, entry in enumerate(raw_presets):
        _require(isinstance(entry, dict), f"projectile preset {index} is not an object")
        identifier = entry.get("id", f"<index {index}>")
        _require(
            isinstance(identifier, str) and identifier.strip() != "",
            f"projectile preset {index} has an empty id",
        )
        _require(identifier not in seen, f"duplicate projectile preset id {identifier!r}")
        seen.add(identifier)
        presets.append(
            ProjectilePreset(
                identifier=identifier,
                name=_read_text(entry, "name", identifier),
                category=_read_text(entry, "category", identifier),
                mass_kg=_read_number(entry, "mass_kg", identifier, MASS_RANGE_KG),
                diameter_m=_read_number(entry, "diameter_m", identifier, DIAMETER_RANGE_M),
                drag_coefficient=_read_number(
                    entry, "drag_coefficient", identifier, DRAG_COEFFICIENT_RANGE
                ),
                muzzle_speed_mps=_read_number(
                    entry, "muzzle_speed_mps", identifier, MUZZLE_SPEED_RANGE_MPS
                ),
                reference_distance_m=_read_number(
                    entry, "reference_distance_m", identifier, (1.0, 20000.0)
                ),
                reference_speed_mps=_read_number(
                    entry, "reference_speed_mps", identifier, (0.0, 1500.0)
                ),
                description=_read_text(entry, "description", identifier),
            )
        )
    return tuple(presets)


def load_projectile_presets(path: Path | None = None) -> tuple[ProjectilePreset, ...]:
    """Load and validate the committed projectile presets."""

    resolved = PRESET_PATH if path is None else path
    if not resolved.is_file():
        raise RuntimeError(f"projectile preset file is missing: {resolved}")
    try:
        document = json.loads(resolved.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        raise RuntimeError(f"projectile preset file is not valid JSON: {resolved}: {error}") from error
    try:
        return parse_projectile_presets(document)
    except ValueError as error:
        raise RuntimeError(f"projectile preset file is invalid: {resolved}: {error}") from error
