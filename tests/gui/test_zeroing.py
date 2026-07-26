from __future__ import annotations

from dataclasses import replace
import math
from pathlib import Path
import unittest

from apps.ballistics_gui.render.scoring import score_target_plane
from apps.ballistics_gui.simulation.cli_bridge import run_cli_scenario
from apps.ballistics_gui.simulation.models import ScenarioConfig
from apps.ballistics_gui.simulation.zeroing import (
    ZeroSolution,
    apply_zero,
    solve_zero_offset,
    zero_cache_key,
)


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
CLI_PATH = REPOSITORY_ROOT / "build" / "apps" / "ballistic_cli" / "ballistics_cli"
CAMERA_HEIGHT_M = 1.58
TARGET_HEIGHT_M = 2.0


def _base_config(**overrides) -> ScenarioConfig:
    config = ScenarioConfig(
        integrator="rk4.v1",
        time_step_s=0.0005,
        maximum_time_s=20.0,
        maximum_distance_m=20000.0,
        projectile_mass_kg=0.018,
        projectile_diameter_m=0.009,
        reference_area_m2=6.3617e-5,
        launch_speed_mps=310.0,
        elevation_deg=0.0,
        azimuth_deg=0.0,
        initial_height_m=1.5,
        drag_coefficient=0.29,
        air_density_kgpm3=1.225,
        wind_x_mps=0.0,
        wind_y_mps=0.0,
        wind_z_mps=0.0,
        gravity_mps2=9.80665,
        target_distance_m=150.0,
    )
    return replace(config, **overrides) if overrides else config


def _aim_at_target_centre(distance_m: float) -> float:
    """Elevation that puts the crosshair on the target centre at one distance."""

    return math.degrees(math.atan2(TARGET_HEIGHT_M - CAMERA_HEIGHT_M, distance_m))


class ZeroSolutionUnitTests(unittest.TestCase):
    """Pure behaviour of the solution record, independent of the solver."""

    def test_angle_conversions(self) -> None:
        solution = ZeroSolution(1.0, 150.0, 2, 0.0, True)
        self.assertAlmostEqual(solution.offset_mil, math.radians(1.0) * 1000.0, places=9)
        self.assertAlmostEqual(solution.offset_moa, 60.0, places=9)

    def test_apply_zero_raises_the_bore(self) -> None:
        config = _base_config(elevation_deg=0.5)
        applied = apply_zero(config, ZeroSolution(0.25, 150.0, 2, 0.0, True))
        self.assertAlmostEqual(applied.elevation_deg, 0.75, places=9)

    def test_apply_zero_is_inert_when_unconverged(self) -> None:
        """An unusable zero must leave the shot alone rather than bias it silently."""

        config = _base_config(elevation_deg=0.5)
        applied = apply_zero(config, ZeroSolution(9.0, 150.0, 8, 4.0, False, "failed"))
        self.assertAlmostEqual(applied.elevation_deg, 0.5, places=9)

    def test_apply_zero_preserves_every_other_field(self) -> None:
        config = _base_config(elevation_deg=0.5)
        applied = apply_zero(config, ZeroSolution(0.25, 150.0, 2, 0.0, True))
        self.assertEqual(
            replace(applied, elevation_deg=0.0), replace(config, elevation_deg=0.0)
        )

    def test_cache_key_ignores_aim_and_crosswind(self) -> None:
        """The bore-to-sight angle is mechanical; it does not change with where the rifle
        points, and a zero is established in still air."""

        base = _base_config()
        moved = replace(base, elevation_deg=3.0, azimuth_deg=-2.0, wind_y_mps=7.0)
        self.assertEqual(
            zero_cache_key(base, 150.0, CAMERA_HEIGHT_M),
            zero_cache_key(moved, 150.0, CAMERA_HEIGHT_M),
        )

    def test_cache_key_tracks_the_load_and_distance(self) -> None:
        base = _base_config()
        for changed in (
            replace(base, launch_speed_mps=800.0),
            replace(base, projectile_mass_kg=0.05),
            replace(base, drag_coefficient=1.0),
            replace(base, gravity_mps2=1.62),
        ):
            with self.subTest(changed=changed.launch_speed_mps):
                self.assertNotEqual(
                    zero_cache_key(base, 150.0, CAMERA_HEIGHT_M),
                    zero_cache_key(changed, 150.0, CAMERA_HEIGHT_M),
                )
        self.assertNotEqual(
            zero_cache_key(base, 150.0, CAMERA_HEIGHT_M),
            zero_cache_key(base, 300.0, CAMERA_HEIGHT_M),
        )

    def test_non_positive_zero_distance_is_refused(self) -> None:
        solution = solve_zero_offset(_base_config(), CLI_PATH, 0.0, CAMERA_HEIGHT_M)
        self.assertFalse(solution.converged)


@unittest.skipUnless(CLI_PATH.is_file(), "ballistics_cli has not been built")
class ZeroSolverTests(unittest.TestCase):
    """The solver drives the real C core, so these assert against actual trajectories."""

    def test_zero_puts_impact_on_the_crosshair_at_the_zero_distance(self) -> None:
        for zero_distance in (100.0, 150.0, 200.0):
            with self.subTest(zero_distance=zero_distance):
                config = _base_config(target_distance_m=zero_distance)
                solution = solve_zero_offset(config, CLI_PATH, zero_distance, CAMERA_HEIGHT_M)
                self.assertTrue(solution.converged, solution.message)

                aimed = replace(config, elevation_deg=_aim_at_target_centre(zero_distance))
                result = run_cli_scenario(apply_zero(aimed, solution), CLI_PATH)
                score = score_target_plane(result.samples, zero_distance)
                self.assertTrue(score.reached_target)
                self.assertLess(abs(score.vertical_error_m), 0.05)

    def test_unzeroed_shot_lands_well_below_the_crosshair(self) -> None:
        """The defect this feature fixes: without a zero the drop is the whole error."""

        config = _base_config(elevation_deg=_aim_at_target_centre(150.0))
        score = score_target_plane(run_cli_scenario(config, CLI_PATH).samples, 150.0)
        self.assertLess(score.vertical_error_m, -0.5)

    def test_bore_sits_above_the_sight_line(self) -> None:
        solution = solve_zero_offset(_base_config(), CLI_PATH, 150.0, CAMERA_HEIGHT_M)
        self.assertTrue(solution.converged)
        self.assertGreater(solution.offset_deg, 0.0)

    def test_a_faster_load_needs_less_elevation(self) -> None:
        slow = solve_zero_offset(_base_config(launch_speed_mps=310.0), CLI_PATH, 150.0, CAMERA_HEIGHT_M)
        fast = solve_zero_offset(_base_config(launch_speed_mps=800.0), CLI_PATH, 150.0, CAMERA_HEIGHT_M)
        self.assertTrue(slow.converged and fast.converged)
        self.assertLess(fast.offset_deg, slow.offset_deg)

    def test_a_longer_zero_needs_more_elevation(self) -> None:
        near = solve_zero_offset(_base_config(), CLI_PATH, 100.0, CAMERA_HEIGHT_M)
        far = solve_zero_offset(_base_config(), CLI_PATH, 250.0, CAMERA_HEIGHT_M)
        self.assertTrue(near.converged and far.converged)
        self.assertGreater(far.offset_deg, near.offset_deg)

    def test_shot_rises_before_the_zero_and_falls_after(self) -> None:
        """A zeroed trajectory crosses the sight line once on the way up and once on the way
        down, so it sits above the reticle short of the zero and below it beyond."""

        zero_distance = 150.0
        config = _base_config()
        solution = solve_zero_offset(config, CLI_PATH, zero_distance, CAMERA_HEIGHT_M)
        self.assertTrue(solution.converged)

        near = replace(config, target_distance_m=100.0, elevation_deg=_aim_at_target_centre(100.0))
        far = replace(config, target_distance_m=200.0, elevation_deg=_aim_at_target_centre(200.0))
        near_error = score_target_plane(
            run_cli_scenario(apply_zero(near, solution), CLI_PATH).samples, 100.0
        ).vertical_error_m
        far_error = score_target_plane(
            run_cli_scenario(apply_zero(far, solution), CLI_PATH).samples, 200.0
        ).vertical_error_m
        self.assertGreater(near_error, 0.0)
        self.assertLess(far_error, 0.0)

    def test_unreachable_zero_distance_reports_why(self) -> None:
        config = _base_config(launch_speed_mps=60.0)
        solution = solve_zero_offset(config, CLI_PATH, 900.0, CAMERA_HEIGHT_M)
        self.assertFalse(solution.converged)
        self.assertTrue(solution.message)
        self.assertAlmostEqual(solution.offset_deg, 0.0)

    def test_solver_converges_quickly(self) -> None:
        solution = solve_zero_offset(_base_config(), CLI_PATH, 150.0, CAMERA_HEIGHT_M)
        self.assertTrue(solution.converged)
        self.assertLessEqual(solution.iterations, 4)


if __name__ == "__main__":
    unittest.main()
