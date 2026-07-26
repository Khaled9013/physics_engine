from __future__ import annotations

import unittest

from apps.ballistics_gui.render import range_layout as layout
from apps.ballistics_gui.render.terrain import terrain_height


class TerrainReliefTests(unittest.TestCase):
    """The lane corridor must stay flat so relief never occludes a shot or a target."""

    def test_lane_corridor_is_flat(self) -> None:
        for downrange in (0.0, 50.0, 150.0, 500.0, 1000.0):
            for lateral in (-layout.LANE_HALF_WIDTH_M, 0.0, layout.LANE_HALF_WIDTH_M):
                with self.subTest(lateral=lateral, downrange=downrange):
                    self.assertEqual(terrain_height(lateral, downrange, 1.0), 0.0)

    def test_firing_position_is_flat(self) -> None:
        self.assertEqual(terrain_height(0.0, 0.0, 1.0), 0.0)
        self.assertEqual(terrain_height(0.0, -20.0, 1.0), 0.0)

    def test_relief_appears_away_from_the_lane(self) -> None:
        samples = [abs(terrain_height(x, 140.0, 1.0)) for x in (80.0, -110.0, 160.0)]
        self.assertTrue(any(value > 0.05 for value in samples), samples)

    def test_relief_is_deterministic(self) -> None:
        first = [terrain_height(x * 7.0, x * 11.0, 1.0) for x in range(40)]
        second = [terrain_height(x * 7.0, x * 11.0, 1.0) for x in range(40)]
        self.assertEqual(first, second)

    def test_zero_amplitude_is_exactly_flat(self) -> None:
        self.assertEqual(terrain_height(120.0, 400.0, 0.0), 0.0)


class LayoutConsistencyTests(unittest.TestCase):
    def test_terrain_bands_overlap_without_a_gap(self) -> None:
        self.assertLess(layout.FAR_FIELD.y_range[0], layout.NEAR_FIELD.y_range[1])
        self.assertLess(layout.NEAR_FIELD.y_range[0], layout.LANE_START_M)

    def test_far_band_encloses_the_near_band_laterally(self) -> None:
        self.assertLessEqual(layout.FAR_FIELD.x_range[0], layout.NEAR_FIELD.x_range[0])
        self.assertGreaterEqual(layout.FAR_FIELD.x_range[1], layout.NEAR_FIELD.x_range[1])

    def test_lane_covers_every_supported_target_distance(self) -> None:
        self.assertGreaterEqual(layout.LANE_END_M, 1000.0)

    def test_distance_markers_sit_outside_the_lane_surface(self) -> None:
        markers = layout.distance_markers()
        self.assertEqual(len(markers), 40)
        for marker in markers:
            with self.subTest(marker=marker.name, y=marker.position[1]):
                self.assertGreater(abs(marker.position[0]), layout.LANE_HALF_WIDTH_M)

    def test_grass_tufts_avoid_the_firing_position(self) -> None:
        for x, y, _scale, _heading in layout.grass_tufts():
            with self.subTest(x=x, y=y):
                self.assertFalse(abs(x) < 7.2 and -8.0 < y < 6.0)

    def test_grass_tufts_are_deterministic(self) -> None:
        self.assertEqual(layout.grass_tufts(), layout.grass_tufts())

    def test_distant_ridges_sit_beyond_the_lane(self) -> None:
        for ridge in layout.RIDGES:
            with self.subTest(ridge=ridge.position):
                self.assertGreater(ridge.position[1], layout.LANE_END_M)

    def test_berms_clear_the_flat_lane_corridor(self) -> None:
        for berm in layout.BERMS[:2]:
            with self.subTest(berm=berm.position):
                inner_edge = abs(berm.position[0]) - berm.scale[0]
                self.assertGreater(inner_edge, layout.LANE_HALF_WIDTH_M)

    def test_terrain_bands_declare_a_usable_tint(self) -> None:
        for band in (layout.NEAR_FIELD, layout.FAR_FIELD):
            with self.subTest(band=band.name):
                self.assertEqual(len(band.tint), 4)
                self.assertTrue(all(channel > 0.0 for channel in band.tint))

    def test_layout_module_has_no_render_dependency(self) -> None:
        source = (
            __import__("pathlib")
            .Path(layout.__file__)
            .read_text(encoding="utf-8")
        )
        self.assertNotIn("panda3d", source)


if __name__ == "__main__":
    unittest.main()
