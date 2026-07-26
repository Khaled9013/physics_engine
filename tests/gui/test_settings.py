from __future__ import annotations

import unittest

from apps.ballistics_gui.render.range_scene import (
    DEFAULT_SENSITIVITY_SETTING,
    SCOPE_SENSITIVITY_RATIO,
    SENSITIVITY_DEGREES_MAX,
    SENSITIVITY_DEGREES_MIN,
    SENSITIVITY_SETTING_MAX,
    SENSITIVITY_SETTING_MIN,
    sensitivity_to_degrees,
)
from apps.ballistics_gui.ui.settings_dialog import (
    WINDOW_SIZE_PRESETS,
    describe_sensitivity,
)


class SensitivityMappingTests(unittest.TestCase):
    def test_endpoints_map_to_the_declared_range(self) -> None:
        self.assertAlmostEqual(
            sensitivity_to_degrees(SENSITIVITY_SETTING_MIN), SENSITIVITY_DEGREES_MIN
        )
        self.assertAlmostEqual(
            sensitivity_to_degrees(SENSITIVITY_SETTING_MAX), SENSITIVITY_DEGREES_MAX
        )

    def test_mapping_is_monotonic(self) -> None:
        values = [sensitivity_to_degrees(float(step)) for step in range(0, 101)]
        for lower, higher in zip(values, values[1:]):
            self.assertLess(lower, higher)

    def test_settings_outside_the_range_are_clamped(self) -> None:
        self.assertAlmostEqual(sensitivity_to_degrees(-40.0), SENSITIVITY_DEGREES_MIN)
        self.assertAlmostEqual(sensitivity_to_degrees(1e6), SENSITIVITY_DEGREES_MAX)

    def test_each_step_changes_speed_by_a_constant_proportion(self) -> None:
        """A geometric map is what keeps the slow end of the slider usable."""

        ratios = [
            sensitivity_to_degrees(float(step + 1)) / sensitivity_to_degrees(float(step))
            for step in range(0, 100)
        ]
        for ratio in ratios:
            self.assertAlmostEqual(ratio, ratios[0], places=9)

    def test_default_is_slower_than_the_previous_fixed_speed(self) -> None:
        """The fixed hip speed this replaced was 0.048 deg per count, reported as too fast."""

        self.assertLess(sensitivity_to_degrees(DEFAULT_SENSITIVITY_SETTING), 0.048)

    def test_default_sits_inside_the_slider_range(self) -> None:
        self.assertGreaterEqual(DEFAULT_SENSITIVITY_SETTING, SENSITIVITY_SETTING_MIN)
        self.assertLessEqual(DEFAULT_SENSITIVITY_SETTING, SENSITIVITY_SETTING_MAX)

    def test_optic_turns_slower_than_hip(self) -> None:
        self.assertLess(SCOPE_SENSITIVITY_RATIO, 1.0)
        self.assertGreater(SCOPE_SENSITIVITY_RATIO, 0.0)


class SettingsPresentationTests(unittest.TestCase):
    def test_readout_reports_setting_and_resolved_speed(self) -> None:
        text = describe_sensitivity(35.0, sensitivity_to_degrees(35.0))
        self.assertIn("35", text)
        self.assertIn("°/count", text)

    def test_readout_covers_every_slider_position(self) -> None:
        for step in range(0, 101):
            with self.subTest(setting=step):
                text = describe_sensitivity(float(step), sensitivity_to_degrees(float(step)))
                self.assertTrue(text.strip())

    def test_window_presets_are_sane_and_ascending(self) -> None:
        self.assertGreaterEqual(len(WINDOW_SIZE_PRESETS), 3)
        widths = [width for _label, width, _height in WINDOW_SIZE_PRESETS]
        self.assertEqual(widths, sorted(widths))
        for label, width, height in WINDOW_SIZE_PRESETS:
            with self.subTest(preset=label):
                self.assertGreater(width, 0)
                self.assertGreater(height, 0)
                self.assertIn(str(width), label)


if __name__ == "__main__":
    unittest.main()
