"""Embedded Panda3D first-person fictional target range."""

from __future__ import annotations

from bisect import bisect_left
from pathlib import Path
from typing import Callable

from direct.gui.OnscreenText import OnscreenText
from panda3d.core import (
    ClockObject,
    Filename,
    LineSegs,
    TextNode,
    TransparencyAttrib,
)

from ..simulation.models import ShotResult
from .assets import require_asset, visual_asset_paths
from .coordinates import sample_to_panda
from .environment import RangeEnvironment
from .lighting import RangeLighting
from .materials import apply_flat_material
from .procedural import (
    add_box,
    add_fullscreen_texture,
    add_sphere,
    add_textured_card,
    make_impact_decal_texture,
    make_scope_texture,
)
from .scoring import TargetScore, score_target_plane
from .sky import HAZE_COLOR, RangeSky
from .view_model import PrecisionViewModel


HIP_FIELD_OF_VIEW_DEG = 68.0
SCOPE_FIELD_OF_VIEW_DEG = 11.5

# A distant near plane is affordable now that the first-person rig no longer needs the
# camera to resolve geometry a few centimetres away, and it is what allows the far plane to
# reach the distant ridges without losing depth precision across the lane.
WORLD_NEAR_M = 0.30
WORLD_FAR_M = 12000.0

TARGET_CENTRE_HEIGHT_M = 2.0
TARGET_FACE_HALF_M = 0.85
IMPACT_DECAL_COUNT = 12

# Aim speed is exposed to the user as a 0-100 setting rather than as degrees per count. The
# setting maps geometrically onto the range below, so each step changes speed by a constant
# proportion; a linear map would crowd every usable slow value into the bottom few steps.
SENSITIVITY_SETTING_MIN = 0.0
SENSITIVITY_SETTING_MAX = 100.0
SENSITIVITY_DEGREES_MIN = 0.0035
SENSITIVITY_DEGREES_MAX = 0.150
DEFAULT_SENSITIVITY_SETTING = 35.0
# Behind the optic the same hand movement must cover far less angle, or the magnified view is
# unusable. This is the ratio the fixed hip and scope speeds previously used.
SCOPE_SENSITIVITY_RATIO = 1.0 / 3.0


def sensitivity_to_degrees(setting: float) -> float:
    """Map the 0-100 aim-speed setting onto degrees of aim per mouse count."""

    clamped = max(SENSITIVITY_SETTING_MIN, min(SENSITIVITY_SETTING_MAX, setting))
    fraction = clamped / SENSITIVITY_SETTING_MAX
    ratio = SENSITIVITY_DEGREES_MAX / SENSITIVITY_DEGREES_MIN
    return SENSITIVITY_DEGREES_MIN * (ratio**fraction)


class RangeScene:
    """Own all Panda3D nodes and animation state for one embedded viewport."""

    def __init__(
        self,
        base,
        on_fire: Callable[[], None],
        on_aim_changed: Callable[[float, float], None],
        on_scope_changed: Callable[[bool], None],
        on_settings_requested: Callable[[], None] | None = None,
    ) -> None:
        self.base = base
        self.on_fire = on_fire
        self.on_aim_changed = on_aim_changed
        self.on_scope_changed = on_scope_changed
        self.on_settings_requested = on_settings_requested
        self.sensitivity_setting = DEFAULT_SENSITIVITY_SETTING
        self.clock = ClockObject.getGlobalClock()
        self.azimuth_deg = 0.0
        self.elevation_deg = 0.2
        self.scope_enabled = False
        self.scope_blend = 0.0
        self.mouse_captured = False
        self.target_distance_m = 150.0
        self.trajectory_node = None
        self.shot_result: ShotResult | None = None
        self.shot_times: tuple[float, ...] = ()
        self.shot_started_s = -1.0
        self.shot_playback_rate = 1.0
        self.pending_score: TargetScore | None = None
        self.target_reaction_started_s = -1.0
        self._next_decal_index = 0
        self._last_aim_emit = (self.elevation_deg, self.azimuth_deg)

        assets = visual_asset_paths()
        self.world_root = base.render.attachNewNode("range-world")
        self.effects_root = base.render.attachNewNode("shot-effects")
        self.base.disableMouse()
        self.base.camera.setPos(0.0, 0.0, 1.58)
        self.base.camLens.setFov(HIP_FIELD_OF_VIEW_DEG)
        self.base.camLens.setNearFar(WORLD_NEAR_M, WORLD_FAR_M)
        self.base.setBackgroundColor(*HAZE_COLOR)
        self._initialize_pipeline(assets)
        self.lighting = RangeLighting(base)
        self.sky = RangeSky(base, assets.sky)
        self.environment = RangeEnvironment(base, self.world_root, assets)
        self._build_target()
        self.view_model = PrecisionViewModel(base, assets)
        self._build_overlay()
        self._bind_input()
        self.base.taskMgr.add(self._update, "ballistics-range-update")

    def _initialize_pipeline(self, assets) -> None:
        try:
            import simplepbr
        except ImportError as error:
            raise RuntimeError(
                "panda3d-simplepbr is required; run ./scripts/setup_gui.sh"
            ) from error
        self.pipeline = simplepbr.init(
            render_node=self.base.render,
            window=self.base.win,
            camera_node=self.base.cam,
            taskmgr=self.base.taskMgr,
            msaa_samples=4,
            max_lights=8,
            use_normal_maps=True,
            use_occlusion_maps=True,
            use_emission_maps=True,
            exposure=-1.15,
            enable_shadows=True,
            # Panda3D's `render.setFog` has no effect under the physically-based shader
            # unless the pipeline is compiled with fog support, so distance haze depends on
            # this flag being set here rather than only on the Fog node itself.
            enable_fog=True,
        )
        for face in assets.sky.cube_faces:
            require_asset(face)
        self.pipeline.env_map = simplepbr.EnvMap.from_file_path(
            Filename.fromOsSpecific(str(assets.sky.cube_pattern()))
        )

    def _build_target(self) -> None:
        loader = self.base.loader
        self.target_root = self.world_root.attachNewNode("active-target")
        for lateral in (-0.98, 0.98):
            add_box(
                loader,
                self.target_root,
                "target-post",
                (lateral, 0.0, TARGET_CENTRE_HEIGHT_M * 0.5),
                (0.055, 0.055, TARGET_CENTRE_HEIGHT_M * 0.5),
                (0.23, 0.18, 0.11, 1.0),
            )
        self.target_board = add_box(
            loader,
            self.target_root,
            "target-board",
            (0.0, 0.0, TARGET_CENTRE_HEIGHT_M),
            (0.95, 0.035, 0.95),
            (0.40, 0.39, 0.36, 1.0),
        )
        add_box(
            loader,
            self.target_root,
            "target-cap",
            (0.0, 0.0, TARGET_CENTRE_HEIGHT_M + 1.0),
            (1.0, 0.05, 0.05),
            (0.18, 0.19, 0.18, 1.0),
        )
        self.target_face = add_textured_card(
            self.target_root,
            "target-face",
            (
                -TARGET_FACE_HALF_M,
                TARGET_FACE_HALF_M,
                -TARGET_FACE_HALF_M,
                TARGET_FACE_HALF_M,
            ),
            self.environment.target_face_texture,
        )
        self.target_face.setPos(0.0, -0.038, TARGET_CENTRE_HEIGHT_M)
        self.target_face.setTransparency(TransparencyAttrib.MAlpha)
        apply_flat_material(self.target_face, "target-face-material", (1.0, 1.0, 1.0, 1.0))
        self.target_face.clearColor()

        decal_texture = make_impact_decal_texture()
        self.impact_decals = []
        for index in range(IMPACT_DECAL_COUNT):
            decal = add_textured_card(
                self.target_face,
                f"impact-decal-{index}",
                (-0.030, 0.030, -0.030, 0.030),
                decal_texture,
            )
            decal.setTransparency(TransparencyAttrib.MAlpha)
            decal.setY(-0.006)
            decal.setLightOff(1)
            decal.hide()
            self.impact_decals.append(decal)
        self.set_target_distance(self.target_distance_m)

    def _build_overlay(self) -> None:
        # Sized from the output rather than its window properties so the overlay also
        # builds when the scene is rendered into an offscreen buffer.
        width = max(512, min(1280, self.base.win.getXSize()))
        height = max(384, min(1280, self.base.win.getYSize()))
        self.scope_overlay = add_fullscreen_texture(
            self.base, make_scope_texture(width, height)
        )
        self.scope_overlay.hide()
        self.crosshair = OnscreenText(
            text="+",
            pos=(0.0, -0.018),
            scale=0.038,
            fg=(0.93, 0.95, 0.92, 0.80),
            shadow=(0.0, 0.0, 0.0, 0.75),
            align=TextNode.ACenter,
            mayChange=False,
        )
        self.range_label = OnscreenText(
            parent=self.base.a2dTopLeft,
            text="PRACTICE RANGE  /  150 m",
            pos=(0.055, -0.075),
            scale=0.035,
            fg=(0.93, 0.95, 0.94, 0.90),
            shadow=(0.0, 0.0, 0.0, 0.80),
            align=TextNode.ALeft,
            mayChange=True,
        )
        self.instructions = OnscreenText(
            text="CLICK TO AIM     LMB FIRE     RMB OPTIC     ESC SETTINGS",
            pos=(0.0, -0.92),
            scale=0.034,
            fg=(0.92, 0.94, 0.93, 0.9),
            shadow=(0.0, 0.0, 0.0, 0.85),
            align=TextNode.ACenter,
            mayChange=True,
        )

    def _bind_input(self) -> None:
        self.base.accept("mouse1", self._mouse_fire)
        self.base.accept("mouse3", self._mouse_scope)
        self.base.accept("escape", self._escape)

    def _escape(self) -> None:
        """Release aim and hand control back to the shell, which opens settings."""

        self.release_mouse()
        if self.on_settings_requested is not None:
            self.on_settings_requested()

    def set_sensitivity_setting(self, setting: float) -> None:
        self.sensitivity_setting = max(
            SENSITIVITY_SETTING_MIN, min(SENSITIVITY_SETTING_MAX, setting)
        )

    def _mouse_fire(self) -> None:
        if not self.mouse_captured:
            self.capture_mouse()
            return
        self.on_fire()

    def _mouse_scope(self) -> None:
        if not self.mouse_captured:
            self.capture_mouse()
        self.toggle_scope()

    def capture_mouse(self) -> None:
        self.mouse_captured = True
        self.instructions.setText("AIM ACTIVE     LMB FIRE     RMB OPTIC     ESC SETTINGS")
        properties = self.base.win.getProperties()
        self.base.win.movePointer(
            0, properties.getXSize() // 2, properties.getYSize() // 2
        )

    def release_mouse(self) -> None:
        self.mouse_captured = False
        self.instructions.setText(
            "CLICK TO AIM     LMB FIRE     RMB OPTIC     ESC SETTINGS"
        )

    def toggle_scope(self) -> None:
        self.scope_enabled = not self.scope_enabled
        if self.scope_enabled:
            self.scope_overlay.show()
        self.on_scope_changed(self.scope_enabled)

    def set_target_distance(self, distance_m: float) -> None:
        self.target_distance_m = distance_m
        self.target_root.setPos(0.0, distance_m, 0.0)
        self.lighting.set_focus_distance(distance_m)
        if hasattr(self, "range_label"):
            self.range_label.setText(f"PRACTICE RANGE  /  {distance_m:.0f} m")

    def set_aim(self, elevation_deg: float, azimuth_deg: float) -> None:
        self.elevation_deg = max(-10.0, min(85.0, elevation_deg))
        self.azimuth_deg = max(-90.0, min(90.0, azimuth_deg))

    def play_shot(self, result: ShotResult) -> TargetScore:
        self.set_target_distance(result.scenario.target_distance_m)
        self.reset_target_color()
        self._replace_trajectory(result)
        self.shot_result = result
        self.shot_times = tuple(sample.time_s for sample in result.samples)
        self.shot_started_s = self.clock.getFrameTime()
        duration = max(result.summary.final_time_s, 0.001)
        self.shot_playback_rate = max(1.0, duration / 2.5)
        self.view_model.fire(self.shot_started_s)
        self.pending_score = score_target_plane(
            result.samples, result.scenario.target_distance_m
        )
        return self.pending_score

    def _replace_trajectory(self, result: ShotResult) -> None:
        if self.trajectory_node is not None:
            self.trajectory_node.removeNode()
        lines = LineSegs("trajectory-path")
        lines.setThickness(1.35)
        lines.setColor(0.22, 0.70, 0.76, 0.48)
        step = max(1, len(result.samples) // 1800)
        selected = result.samples[::step]
        if selected[-1] is not result.samples[-1]:
            selected = (*selected, result.samples[-1])
        lines.moveTo(*sample_to_panda(selected[0]))
        for sample in selected[1:]:
            lines.drawTo(*sample_to_panda(sample))
        self.trajectory_node = self.effects_root.attachNewNode(lines.create())
        self.trajectory_node.setLightOff(1)
        self.trajectory_node.setFogOff(1)
        self.trajectory_node.hide()
        if not hasattr(self, "tracer"):
            self.tracer = add_sphere(
                self.base.loader,
                self.effects_root,
                "tracer",
                (0.0, 0.0, 0.0),
                (0.055, 0.055, 0.055),
                (1.0, 0.64, 0.12, 1.0),
            )
            apply_flat_material(
                self.tracer,
                "tracer-emissive",
                (1.0, 0.45, 0.05, 1.0),
                roughness=0.4,
                emission=(1.9, 0.62, 0.08, 1.0),
            )
            self.tracer.setLightOff(1)
            self.impact_marker = add_sphere(
                self.base.loader,
                self.effects_root,
                "impact",
                (0.0, 0.0, 0.0),
                (0.14, 0.14, 0.14),
                (0.42, 0.40, 0.36, 1.0),
            )
        self.tracer.show()
        self.impact_marker.hide()

    def _mark_impact(self, score: TargetScore) -> None:
        """Place a bullet-hole decal on the target face where the shot crossed it."""

        if score.lateral_error_m is None or score.vertical_error_m is None:
            return
        if (
            abs(score.lateral_error_m) > TARGET_FACE_HALF_M
            or abs(score.vertical_error_m) > TARGET_FACE_HALF_M
        ):
            return
        decal = self.impact_decals[self._next_decal_index]
        self._next_decal_index = (self._next_decal_index + 1) % len(self.impact_decals)
        decal.setPos(score.lateral_error_m, -0.006, score.vertical_error_m)
        decal.show()

    def reset_target_color(self) -> None:
        self.target_board.setColor(0.40, 0.39, 0.36, 1.0)
        self.target_root.setP(0.0)
        self.target_reaction_started_s = -1.0

    def reset(self) -> None:
        self.release_mouse()
        self.scope_enabled = False
        self.scope_blend = 0.0
        self.scope_overlay.hide()
        self.crosshair.show()
        self.instructions.show()
        self.base.camLens.setFov(HIP_FIELD_OF_VIEW_DEG)
        self.reset_target_color()
        self.view_model.reset()
        self.shot_result = None
        self.pending_score = None
        for decal in self.impact_decals:
            decal.hide()
        self._next_decal_index = 0
        if self.trajectory_node is not None:
            self.trajectory_node.removeNode()
            self.trajectory_node = None
        if hasattr(self, "tracer"):
            self.tracer.hide()
            self.impact_marker.hide()

    def _update(self, task):
        now = self.clock.getFrameTime()
        self._update_mouse()
        self._update_scope_and_rig(now)
        self._update_shot(now)
        self._update_target_reaction(now)
        return task.cont

    def _update_mouse(self) -> None:
        if not self.mouse_captured or self.base.win is None:
            return
        properties = self.base.win.getProperties()
        center_x = properties.getXSize() // 2
        center_y = properties.getYSize() // 2
        pointer = self.base.win.getPointer(0)
        delta_x = pointer.getX() - center_x
        delta_y = pointer.getY() - center_y
        if delta_x == 0 and delta_y == 0:
            return
        sensitivity = sensitivity_to_degrees(self.sensitivity_setting)
        if self.scope_enabled:
            sensitivity *= SCOPE_SENSITIVITY_RATIO
        self.azimuth_deg = max(
            -50.0, min(50.0, self.azimuth_deg + delta_x * sensitivity)
        )
        self.elevation_deg = max(
            -8.0, min(30.0, self.elevation_deg - delta_y * sensitivity)
        )
        self.base.win.movePointer(0, center_x, center_y)
        aim = (self.elevation_deg, self.azimuth_deg)
        if aim != self._last_aim_emit:
            self._last_aim_emit = aim
            self.on_aim_changed(*aim)

    def _update_scope_and_rig(self, now: float) -> None:
        target = 1.0 if self.scope_enabled else 0.0
        blend_speed = min(1.0, self.clock.getDt() * 8.0)
        self.scope_blend += (target - self.scope_blend) * blend_speed
        self.base.camLens.setFov(
            HIP_FIELD_OF_VIEW_DEG
            + (SCOPE_FIELD_OF_VIEW_DEG - HIP_FIELD_OF_VIEW_DEG) * self.scope_blend
        )
        self.scope_overlay.setColorScale(1.0, 1.0, 1.0, self.scope_blend)
        if self.scope_blend < 0.01 and not self.scope_enabled:
            self.scope_overlay.hide()
        self.crosshair.setColorScale(1.0, 1.0, 1.0, 1.0 - self.scope_blend)
        rig_visible = self.scope_blend < 0.84
        self.view_model.set_visible(rig_visible)
        self.instructions.setColorScale(
            1.0, 1.0, 1.0, max(0.0, 1.0 - self.scope_blend * 1.4)
        )
        self.view_model.update(now, self.clock.getDt(), self.scope_blend)
        self.base.camera.setHpr(-self.azimuth_deg, self.elevation_deg, 0.0)

    def _update_shot(self, now: float) -> None:
        if self.shot_result is None or self.shot_started_s < 0.0:
            return
        simulated_time = (now - self.shot_started_s) * self.shot_playback_rate
        final_time = self.shot_times[-1]
        if simulated_time >= final_time:
            final_sample = self.shot_result.samples[-1]
            self.tracer.setPos(*sample_to_panda(final_sample))
            self.tracer.hide()
            self.impact_marker.setPos(*sample_to_panda(final_sample))
            self.impact_marker.show()
            self.trajectory_node.show()
            if self.pending_score is not None:
                if self.pending_score.hit:
                    self._mark_impact(self.pending_score)
                    self.target_board.setColor(0.20, 0.44, 0.26, 1.0)
                    self.target_reaction_started_s = now
                elif self.pending_score.reached_target:
                    self._mark_impact(self.pending_score)
                    self.target_board.setColor(0.52, 0.32, 0.14, 1.0)
                else:
                    self.target_board.setColor(0.30, 0.31, 0.32, 1.0)
            self.shot_started_s = -1.0
            return
        index = bisect_left(self.shot_times, simulated_time)
        index = max(1, min(index, len(self.shot_times) - 1))
        previous = self.shot_result.samples[index - 1]
        current = self.shot_result.samples[index]
        delta = current.time_s - previous.time_s
        alpha = (
            0.0
            if delta == 0.0
            else (simulated_time - previous.time_s) / delta
        )
        p0 = sample_to_panda(previous)
        p1 = sample_to_panda(current)
        self.tracer.setPos(
            *(p0[axis] + alpha * (p1[axis] - p0[axis]) for axis in range(3))
        )

    def _update_target_reaction(self, now: float) -> None:
        if self.target_reaction_started_s < 0.0:
            return
        age = now - self.target_reaction_started_s
        if age >= 0.55:
            self.target_root.setP(0.0)
            self.target_reaction_started_s = -1.0
            return
        self.target_root.setP(-9.0 * (1.0 - age / 0.55))

    def save_screenshot(self, path: Path) -> bool:
        return bool(
            self.base.win.saveScreenshot(Filename.fromOsSpecific(str(path)))
        )

    def destroy(self) -> None:
        self.base.taskMgr.remove("ballistics-range-update")
        self.base.ignoreAll()
        self.view_model.destroy()
        self.environment.destroy()
        self.sky.destroy()
        self.lighting.destroy()
        self.scope_overlay.removeNode()
        self.world_root.removeNode()
        self.effects_root.removeNode()
