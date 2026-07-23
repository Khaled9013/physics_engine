from __future__ import annotations

import unittest

from apps.ballistics_gui.render.coordinates import ballistics_to_panda, sample_to_panda
from apps.ballistics_gui.render.scoring import score_target_plane
from apps.ballistics_gui.simulation.models import TrajectorySample


def sample(time_s: float, x: float, y: float, z: float) -> TrajectorySample:
    return TrajectorySample(time_s, x, y, z, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0)


class CoordinateTests(unittest.TestCase):
    def test_ballistics_axes_map_once_to_panda_axes(self) -> None:
        self.assertEqual(ballistics_to_panda(10.0, 2.0, 3.0), (2.0, 10.0, 3.0))
        self.assertEqual(sample_to_panda(sample(0.0, 10.0, 2.0, 3.0)), (2.0, 10.0, 3.0))


class TargetScoringTests(unittest.TestCase):
    def test_interpolated_plane_crossing_hits_target(self) -> None:
        result = score_target_plane(
            (sample(0.0, 0.0, -1.0, 1.0), sample(1.0, 20.0, 1.0, 3.0)),
            target_distance_m=10.0,
            target_height_m=2.0,
            target_radius_m=0.1,
        )
        self.assertTrue(result.reached_target)
        self.assertTrue(result.hit)
        self.assertIsNotNone(result.radial_error_m)
        self.assertAlmostEqual(result.radial_error_m, 0.0)

    def test_miss_reports_signed_errors(self) -> None:
        result = score_target_plane(
            (sample(0.0, 0.0, 0.0, 2.0), sample(1.0, 20.0, 2.0, 4.0)),
            target_distance_m=10.0,
            target_height_m=2.0,
            target_radius_m=0.25,
        )
        self.assertTrue(result.reached_target)
        self.assertFalse(result.hit)
        self.assertAlmostEqual(result.lateral_error_m or 0.0, 1.0)
        self.assertAlmostEqual(result.vertical_error_m or 0.0, 1.0)

    def test_short_trajectory_does_not_invent_intersection(self) -> None:
        result = score_target_plane(
            (sample(0.0, 0.0, 0.0, 2.0), sample(1.0, 5.0, 0.0, 2.0)),
            target_distance_m=10.0,
        )
        self.assertFalse(result.reached_target)
        self.assertFalse(result.hit)
        self.assertIsNone(result.radial_error_m)

    def test_invalid_target_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            score_target_plane((sample(0.0, 0.0, 0.0, 0.0),), 10.0)


if __name__ == "__main__":
    unittest.main()
