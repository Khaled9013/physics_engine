"""Embedded Panda3D first-person fictional target range."""

from __future__ import annotations

from bisect import bisect_left
from pathlib import Path
from typing import Callable

from direct.gui.OnscreenText import OnscreenText
from panda3d.core import (
    AmbientLight,
    ClockObject,
    DirectionalLight,
    Filename,
    Fog,
    LineSegs,
    TextNode,
)

from ..simulation.models import ShotResult
from .assets import visual_asset_paths
from .coordinates import sample_to_panda
from .environment import RangeEnvironment
from .procedural import (
    add_box,
    add_fullscreen_texture,
    add_sphere,
    apply_material,
    make_scope_texture,
)
from .scoring import TargetScore, score_target_plane
from .view_model import PrecisionViewModel


class RangeScene:
    """Own all Panda3D nodes and animation state for one embedded viewport."""

    def __init__(
        self,
        base,
        on_fire: Callable[[], None],
        on_aim_changed: Callable[[float, float], None],
        on_scope_changed: Callable[[bool], None],
    ) -> None:
        self.base = base
        self.on_fire = on_fire
        self.on_aim_changed = on_aim_changed
        self.on_scope_changed = on_scope_changed
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
        self._last_aim_emit = (self.elevation_deg, self.azimuth_deg)

        self.world_root = base.render.attachNewNode("range-world")
        self.effects_root = base.render.attachNewNode("shot-effects")
        self.base.disableMouse()
        self.base.camera.setPos(0.0, 0.0, 1.55)
        self.base.camLens.setFov(70.0)
        self.base.camLens.setNearFar(0.025, 2500.0)
        self.base.setBackgroundColor(0.19, 0.36, 0.52)
        self._initialize_pipeline()
        self._build_lighting()
        self.environment = RangeEnvironment(base, self.world_root, visual_asset_paths())
        self._build_target()
        self.view_model = PrecisionViewModel(base, visual_asset_paths())
        self._build_overlay()
        self._bind_input()
        self.base.taskMgr.add(self._update, "ballistics-range-update")

    def _initialize_pipeline(self) -> None:
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
            exposure=-0.15,
            enable_shadows=True,
            enable_fog=False,
        )

    def _build_lighting(self) -> None:
        ambient = AmbientLight("range-ambient")
        ambient.setColor((0.28, 0.31, 0.34, 1.0))
        self.ambient_node = self.base.render.attachNewNode(ambient)
        self.base.render.setLight(self.ambient_node)

        sun = DirectionalLight("range-sun")
        sun.setColorTemperature(5550.0)
        sun.setShadowCaster(True, 1536, 1536)
        self.sun_node = self.base.render.attachNewNode(sun)
        self.sun_node.setHpr(-28.0, -52.0, 0.0)
        self.base.render.setLight(self.sun_node)

        fog = Fog("range-atmospheric-haze")
        fog.setColor(0.30, 0.49, 0.64)
        fog.setLinearRange(520.0, 1500.0)
        self.base.render.setFog(fog)

    def _build_target(self) -> None:
        loader = self.base.loader
        self.target_root = self.world_root.attachNewNode("active-target")
        for lateral in (-1.22, 1.22):
            add_box(
                loader,
                self.target_root,
                "target-post",
                (lateral, 0.0, 1.25),
                (0.075, 0.075, 1.25),
                (0.24, 0.18, 0.10, 1.0),
            )
        add_box(
            loader,
            self.target_root,
            "target-board",
            (0.0, 0.0, 2.0),
            (1.55, 0.10, 1.55),
            (0.75, 0.72, 0.64, 1.0),
        )
        add_box(
            loader,
            self.target_root,
            "target-cap",
            (0.0, 0.0, 3.58),
            (1.65, 0.13, 0.08),
            (0.18, 0.20, 0.19, 1.0),
        )
        ring_data = (
            (0.95, -0.125, (0.11, 0.13, 0.13, 1.0)),
            (0.72, -0.17, (0.78, 0.74, 0.62, 1.0)),
            (0.48, -0.215, (0.72, 0.17, 0.12, 1.0)),
            (0.23, -0.26, (0.92, 0.78, 0.24, 1.0)),
        )
        self.target_rings = []
        for radius, y, color in ring_data:
            ring = add_sphere(
                loader,
                self.target_root,
                "target-scoring-ring",
                (0.0, y, 2.0),
                (radius, 0.035, radius),
                color,
            )
            self.target_rings.append(ring)
        self.target_plate = self.target_rings[0]
        self.target_center = self.target_rings[-1]
        self.set_target_distance(self.target_distance_m)

    def _build_overlay(self) -> None:
        aspect_ratio = max(self.base.getAspectRatio(), 0.5)
        width = 960
        height = max(480, min(960, round(width / aspect_ratio)))
        self.scope_overlay = add_fullscreen_texture(
            self.base, make_scope_texture(width, height)
        )
        self.scope_overlay.hide()
        self.crosshair = OnscreenText(
            text="+",
            pos=(0.0, -0.018),
            scale=0.038,
            fg=(0.73, 0.88, 0.78, 0.86),
            shadow=(0.0, 0.0, 0.0, 0.65),
            align=TextNode.ACenter,
            mayChange=False,
        )
        self.range_label = OnscreenText(
            parent=self.base.a2dTopLeft,
            text="PRACTICE RANGE  /  150 m",
            pos=(0.055, -0.075),
            scale=0.035,
            fg=(0.82, 0.88, 0.86, 0.88),
            shadow=(0.0, 0.0, 0.0, 0.75),
            align=TextNode.ALeft,
            mayChange=True,
        )
        self.instructions = OnscreenText(
            text="CLICK TO AIM     LMB FIRE     RMB OPTIC     ESC RELEASE",
            pos=(0.0, -0.92),
            scale=0.034,
            fg=(0.88, 0.91, 0.90, 0.9),
            shadow=(0.0, 0.0, 0.0, 0.8),
            align=TextNode.ACenter,
            mayChange=True,
        )

    def _bind_input(self) -> None:
        self.base.accept("mouse1", self._mouse_fire)
        self.base.accept("mouse3", self._mouse_scope)
        self.base.accept("escape", self.release_mouse)

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
        self.instructions.setText("AIM ACTIVE     LMB FIRE     RMB OPTIC     ESC RELEASE")
        properties = self.base.win.getProperties()
        self.base.win.movePointer(
            0, properties.getXSize() // 2, properties.getYSize() // 2
        )

    def release_mouse(self) -> None:
        self.mouse_captured = False
        self.instructions.setText(
            "CLICK TO AIM     LMB FIRE     RMB OPTIC     ESC RELEASE"
        )

    def toggle_scope(self) -> None:
        self.scope_enabled = not self.scope_enabled
        if self.scope_enabled:
            self.scope_overlay.show()
        self.on_scope_changed(self.scope_enabled)

    def set_target_distance(self, distance_m: float) -> None:
        self.target_distance_m = distance_m
        self.target_root.setPos(0.0, distance_m, 0.0)
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
        self.trajectory_node.hide()
        if not hasattr(self, "tracer"):
            self.tracer = add_sphere(
                self.base.loader,
                self.effects_root,
                "tracer",
                (0.0, 0.0, 0.0),
                (0.065, 0.065, 0.065),
                (1.0, 0.64, 0.12, 1.0),
            )
            apply_material(
                self.tracer,
                "tracer-emissive",
                (1.0, 0.42, 0.04, 1.0),
                emission=(1.0, 0.18, 0.01, 1.0),
            )
            self.impact_marker = add_sphere(
                self.base.loader,
                self.effects_root,
                "impact",
                (0.0, 0.0, 0.0),
                (0.18, 0.18, 0.18),
                (1.0, 0.18, 0.08, 1.0),
            )
        self.tracer.show()
        self.impact_marker.hide()

    def reset_target_color(self) -> None:
        colors = (
            (0.11, 0.13, 0.13, 1.0),
            (0.78, 0.74, 0.62, 1.0),
            (0.72, 0.17, 0.12, 1.0),
            (0.92, 0.78, 0.24, 1.0),
        )
        for ring, color in zip(self.target_rings, colors, strict=True):
            ring.setColor(*color)
        self.target_root.setP(0.0)
        self.target_reaction_started_s = -1.0

    def reset(self) -> None:
        self.release_mouse()
        self.scope_enabled = False
        self.scope_blend = 0.0
        self.scope_overlay.hide()
        self.crosshair.show()
        self.instructions.show()
        self.base.camLens.setFov(70.0)
        self.reset_target_color()
        self.view_model.reset()
        self.shot_result = None
        self.pending_score = None
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
        sensitivity = 0.016 if self.scope_enabled else 0.048
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
        self.base.camLens.setFov(70.0 + (12.5 - 70.0) * self.scope_blend)
        self.scope_overlay.setColorScale(1.0, 1.0, 1.0, self.scope_blend)
        if self.scope_blend < 0.01 and not self.scope_enabled:
            self.scope_overlay.hide()
        self.crosshair.setColorScale(1.0, 1.0, 1.0, 1.0 - self.scope_blend)
        rig_visible = self.scope_blend < 0.84
        self.view_model.set_visible(rig_visible)
        self.instructions.setColorScale(
            1.0, 1.0, 1.0, max(0.0, 1.0 - self.scope_blend * 1.4)
        )
        self.view_model.update(
            now, self.clock.getDt(), self.scope_blend
        )
        self.base.camera.setHpr(
            -self.azimuth_deg, self.elevation_deg, 0.0
        )

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
                    self.target_rings[0].setColor(0.14, 0.60, 0.30, 1.0)
                    self.target_reaction_started_s = now
                elif self.pending_score.reached_target:
                    self.target_rings[0].setColor(0.90, 0.38, 0.08, 1.0)
                else:
                    self.target_rings[0].setColor(0.32, 0.38, 0.40, 1.0)
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
        self.base.render.clearFog()
        self.base.render.clearLight(self.ambient_node)
        self.base.render.clearLight(self.sun_node)
        self.view_model.destroy()
        self.environment.destroy()
        self.scope_overlay.removeNode()
        self.world_root.removeNode()
        self.effects_root.removeNode()
