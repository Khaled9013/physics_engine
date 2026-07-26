"""Widget-level tests for the scenario controls.

These run against a real `ScenarioControls`, because the two defects they cover — a
sign-inverted headwind and a diameter that never reached the solver — lived in the
translation between the widgets and `ScenarioConfig`, not in either side alone. Qt is driven
on its offscreen platform so no display is required.
"""

from __future__ import annotations

import math
import os
import unittest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

try:
    from PyQt6.QtWidgets import QApplication

    from apps.ballistics_gui.ui.controls import ScenarioControls
except ImportError:  # pragma: no cover - exercised only without PyQt6 installed
    QApplication = None

from apps.ballistics_gui.simulation.projectiles import load_projectile_presets


_application = None


def setUpModule() -> None:
    global _application
    if QApplication is None:
        raise unittest.SkipTest("PyQt6 is not installed")
    _application = QApplication.instance() or QApplication([])


@unittest.skipIf(QApplication is None, "PyQt6 is not installed")
class HeadwindSignTests(unittest.TestCase):
    def setUp(self) -> None:
        self.controls = ScenarioControls()

    def test_positive_headwind_becomes_negative_downrange_wind(self) -> None:
        """A headwind blows toward the shooter, which is the -x direction. Passing the field
        through unchanged made a headwind lengthen the shot instead of shortening it."""

        self.controls._fields["headwind_mps"].setValue(6.0)
        self.assertAlmostEqual(self.controls.scenario().wind_x_mps, -6.0)

    def test_negative_headwind_becomes_a_tailwind(self) -> None:
        self.controls._fields["headwind_mps"].setValue(-4.5)
        self.assertAlmostEqual(self.controls.scenario().wind_x_mps, 4.5)

    def test_zero_headwind_is_zero_wind(self) -> None:
        self.controls._fields["headwind_mps"].setValue(0.0)
        self.assertAlmostEqual(self.controls.scenario().wind_x_mps, 0.0)

    def test_crosswind_is_unaffected(self) -> None:
        self.controls._fields["wind_y_mps"].setValue(3.0)
        self.assertAlmostEqual(self.controls.scenario().wind_y_mps, 3.0)


@unittest.skipIf(QApplication is None, "PyQt6 is not installed")
class ReferenceAreaCouplingTests(unittest.TestCase):
    def setUp(self) -> None:
        self.controls = ScenarioControls()

    def test_diameter_drives_reference_area(self) -> None:
        for diameter in (0.00566, 0.00782, 0.01295, 0.0175):
            with self.subTest(diameter=diameter):
                self.controls._fields["projectile_diameter_m"].setValue(diameter)
                config = self.controls.scenario()
                self.assertAlmostEqual(
                    config.reference_area_m2,
                    math.pi * config.projectile_diameter_m**2 / 4.0,
                    delta=config.reference_area_m2 * 1e-4,
                )

    def test_area_remains_overridable(self) -> None:
        self.controls._fields["projectile_diameter_m"].setValue(0.00900)
        self.controls._fields["reference_area_m2"].setValue(1.0e-4)
        self.assertAlmostEqual(self.controls.scenario().reference_area_m2, 1.0e-4)


@unittest.skipIf(QApplication is None, "PyQt6 is not installed")
class ProjectilePresetTests(unittest.TestCase):
    def setUp(self) -> None:
        self.controls = ScenarioControls()
        self.presets = load_projectile_presets()

    def test_selector_offers_custom_plus_every_preset(self) -> None:
        self.assertEqual(self.controls.preset_box.count(), len(self.presets) + 1)
        self.assertIsNone(self.controls.preset_box.itemData(0))

    def test_every_preset_populates_the_scenario_exactly(self) -> None:
        for index, preset in enumerate(self.presets, start=1):
            with self.subTest(preset=preset.identifier):
                self.controls.preset_box.setCurrentIndex(index)
                config = self.controls.scenario()
                self.assertAlmostEqual(config.projectile_mass_kg, preset.mass_kg, places=9)
                self.assertAlmostEqual(config.projectile_diameter_m, preset.diameter_m, places=9)
                self.assertAlmostEqual(config.drag_coefficient, preset.drag_coefficient, places=9)
                self.assertAlmostEqual(config.launch_speed_mps, preset.muzzle_speed_mps, places=6)
                self.assertAlmostEqual(
                    config.reference_area_m2,
                    preset.reference_area_m2,
                    delta=preset.reference_area_m2 * 1e-4,
                )

    def test_every_preset_produces_a_valid_scenario(self) -> None:
        for index in range(1, self.controls.preset_box.count()):
            with self.subTest(index=index):
                self.controls.preset_box.setCurrentIndex(index)
                self.controls.scenario().validate()

    def test_editing_a_field_returns_the_selector_to_custom(self) -> None:
        self.controls.preset_box.setCurrentIndex(1)
        self.assertIsNotNone(self.controls.preset_box.currentData())
        self.controls._fields["projectile_diameter_m"].setValue(0.02)
        self.assertIsNone(self.controls.preset_box.currentData())

    def test_applying_a_preset_does_not_flip_to_custom(self) -> None:
        for index in range(1, self.controls.preset_box.count()):
            with self.subTest(index=index):
                self.controls.preset_box.setCurrentIndex(index)
                self.assertIsNotNone(self.controls.preset_box.currentData())


if __name__ == "__main__":
    unittest.main()
