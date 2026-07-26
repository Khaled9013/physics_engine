from __future__ import annotations

import copy
import json
import math
from pathlib import Path
import tempfile
import unittest

from apps.ballistics_gui.simulation.projectiles import (
    PRESET_PATH,
    SUPPORTED_SCHEMA_VERSION,
    circular_reference_area_m2,
    load_projectile_presets,
    parse_projectile_presets,
)


def _valid_document() -> dict:
    return {
        "schema_version": SUPPORTED_SCHEMA_VERSION,
        "presets": [
            {
                "id": "example",
                "name": "Example",
                "category": "Rifle",
                "mass_kg": 0.01,
                "diameter_m": 0.008,
                "drag_coefficient": 0.3,
                "muzzle_speed_mps": 800.0,
                "reference_distance_m": 500.0,
                "reference_speed_mps": 500.0,
                "description": "An example.",
            }
        ],
    }


class PresetFileTests(unittest.TestCase):
    def test_committed_presets_load(self) -> None:
        presets = load_projectile_presets()
        self.assertGreaterEqual(len(presets), 6)

    def test_preset_file_is_committed(self) -> None:
        self.assertTrue(PRESET_PATH.is_file(), PRESET_PATH)

    def test_identifiers_and_names_are_unique(self) -> None:
        presets = load_projectile_presets()
        self.assertEqual(len({p.identifier for p in presets}), len(presets))
        self.assertEqual(len({p.name for p in presets}), len(presets))

    def test_reference_area_is_derived_from_diameter(self) -> None:
        for preset in load_projectile_presets():
            with self.subTest(preset=preset.identifier):
                self.assertAlmostEqual(
                    preset.reference_area_m2,
                    math.pi * preset.diameter_m**2 / 4.0,
                    places=12,
                )

    def test_drag_coefficients_reproduce_their_reference_velocity(self) -> None:
        """Each coefficient is fitted to a published remaining velocity; this is the check
        that the fit still holds, and it is what makes the presets falsifiable."""

        for preset in load_projectile_presets():
            with self.subTest(preset=preset.identifier):
                predicted = preset.expected_speed_at_reference()
                relative_error = abs(predicted - preset.reference_speed_mps) / preset.reference_speed_mps
                self.assertLess(relative_error, 0.01, f"{preset.name}: {relative_error:.3%}")

    def test_sectional_densities_are_physically_plausible(self) -> None:
        """A real small-arms projectile sits far below the 4321 kg/m^2 that a 0.35 kg
        9 mm slug would imply, which is the input that prompted these presets."""

        for preset in load_projectile_presets():
            with self.subTest(preset=preset.identifier):
                self.assertLess(preset.sectional_density_kgpm2, 400.0)
                self.assertGreater(preset.sectional_density_kgpm2, 10.0)

    def test_drag_coefficients_are_in_a_credible_band(self) -> None:
        for preset in load_projectile_presets():
            with self.subTest(preset=preset.identifier):
                self.assertGreater(preset.drag_coefficient, 0.15)
                self.assertLess(preset.drag_coefficient, 0.80)

    def test_spheres_have_more_drag_than_bullets(self) -> None:
        presets = load_projectile_presets()
        spheres = [p for p in presets if p.category == "Sphere"]
        bullets = [p for p in presets if p.category != "Sphere"]
        self.assertTrue(spheres and bullets)
        self.assertGreater(
            min(p.drag_coefficient for p in spheres),
            max(p.drag_coefficient for p in bullets),
        )


class PresetValidationTests(unittest.TestCase):
    def test_valid_document_parses(self) -> None:
        self.assertEqual(len(parse_projectile_presets(_valid_document())), 1)

    def test_unsupported_schema_is_rejected(self) -> None:
        document = _valid_document()
        document["schema_version"] = SUPPORTED_SCHEMA_VERSION + 1
        with self.assertRaises(ValueError):
            parse_projectile_presets(document)

    def test_missing_field_is_rejected(self) -> None:
        document = _valid_document()
        del document["presets"][0]["mass_kg"]
        with self.assertRaises(ValueError):
            parse_projectile_presets(document)

    def test_out_of_range_value_is_rejected(self) -> None:
        document = _valid_document()
        document["presets"][0]["drag_coefficient"] = 99.0
        with self.assertRaises(ValueError):
            parse_projectile_presets(document)

    def test_non_numeric_value_is_rejected(self) -> None:
        document = _valid_document()
        document["presets"][0]["mass_kg"] = "heavy"
        with self.assertRaises(ValueError):
            parse_projectile_presets(document)

    def test_duplicate_identifier_is_rejected(self) -> None:
        document = _valid_document()
        document["presets"].append(copy.deepcopy(document["presets"][0]))
        with self.assertRaises(ValueError):
            parse_projectile_presets(document)

    def test_empty_preset_list_is_rejected(self) -> None:
        document = _valid_document()
        document["presets"] = []
        with self.assertRaises(ValueError):
            parse_projectile_presets(document)

    def test_missing_file_reports_its_path(self) -> None:
        missing = Path(tempfile.gettempdir()) / "definitely-not-a-preset-file.json"
        with self.assertRaises(RuntimeError) as caught:
            load_projectile_presets(missing)
        self.assertIn(str(missing), str(caught.exception))

    def test_malformed_json_reports_its_path(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "broken.json"
            path.write_text("{not json", encoding="utf-8")
            with self.assertRaises(RuntimeError) as caught:
                load_projectile_presets(path)
            self.assertIn(str(path), str(caught.exception))

    def test_invalid_document_reports_its_path(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "wrong.json"
            document = _valid_document()
            document["schema_version"] = 99
            path.write_text(json.dumps(document), encoding="utf-8")
            with self.assertRaises(RuntimeError) as caught:
                load_projectile_presets(path)
            self.assertIn(str(path), str(caught.exception))


class CircularAreaTests(unittest.TestCase):
    def test_known_diameter(self) -> None:
        self.assertAlmostEqual(circular_reference_area_m2(0.009), 6.36172512e-5, places=12)

    def test_area_scales_with_the_square_of_diameter(self) -> None:
        self.assertAlmostEqual(
            circular_reference_area_m2(0.02) / circular_reference_area_m2(0.01), 4.0
        )


if __name__ == "__main__":
    unittest.main()
