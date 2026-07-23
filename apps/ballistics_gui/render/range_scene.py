"""Embedded Panda3D first-person fictional target range."""

from __future__ import annotations

from bisect import bisect_left
import math
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
    PointLight,
    TextNode,
)

from ..simulation.models import ShotResult
from .coordinates import sample_to_panda
from .procedural import add_box, add_card, add_fullscreen_texture, add_sphere, make_scope_texture
from .scoring import TargetScore, score_target_plane


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
        self.recoil_started_s = -1.0
        self._last_aim_emit = (self.elevation_deg, self.azimuth_deg)

        self.world_root = base.render.attachNewNode("range-world")
        self.effects_root = base.render.attachNewNode("shot-effects")
        self.base.disableMouse()
        self.base.camera.setPos(0.0, 0.0, 1.5)
        self.base.camLens.setFov(72.0)
        self.base.camLens.setNearFar(0.03, 2500.0)
        self.base.setBackgroundColor(0.34, 0.56, 0.72)
        self._build_lighting()
        self._build_world()
        self._build_target()
        self._build_view_model()
        self._build_overlay()
        self._bind_input()
        self.base.taskMgr.add(self._update, "ballistics-range-update")

    def _build_lighting(self) -> None:
        ambient = AmbientLight("range-ambient")
        ambient.setColor((0.36, 0.39, 0.42, 1.0))
        self.world_root.setLight(self.base.render.attachNewNode(ambient))
        sun = DirectionalLight("range-sun")
        sun.setColor((1.0, 0.92, 0.78, 1.0))
        try:
            sun.setShadowCaster(True, 1024, 1024)
        except TypeError:
            pass
        sun_node = self.base.render.attachNewNode(sun)
        sun_node.setHpr(-32.0, -48.0, 0.0)
        self.world_root.setLight(sun_node)
        self.base.render.setShaderAuto()
        fog = Fog("range-haze")
        fog.setColor(0.34, 0.56, 0.72)
        fog.setLinearRange(450.0, 1200.0)
        self.world_root.setFog(fog)

    def _build_world(self) -> None:
        loader = self.base.loader
        ground = add_card(
            self.world_root, "ground-plane", (-70, 70, 0, 1100), (0.24, 0.31, 0.20, 1)
        )
        ground.setP(-90)
        ground.setZ(-0.01)
        lane = add_card(
            self.world_root, "range-lane", (-6, 6, 0, 1050), (0.47, 0.40, 0.27, 1)
        )
        lane.setP(-90)
        lane.setZ(0.01)
        for lateral in (-6.2, 6.2):
            add_box(loader, self.world_root, "lane-edge", (lateral, 500, 0.04), (0.06, 500, 0.04), (0.82, 0.76, 0.58, 1))
        for distance in range(50, 1001, 50):
            width = 0.12 if distance % 100 else 0.22
            add_box(loader, self.world_root, "range-line", (0, distance, 0.045), (6, width, 0.025), (0.73, 0.68, 0.52, 1))
        for lateral in (-22, 22):
            add_box(loader, self.world_root, "side-berm", (lateral, 500, 2.0), (15, 500, 2.0), (0.20, 0.27, 0.18, 1))
        add_box(loader, self.world_root, "backstop", (0, 1010, 8), (55, 12, 8), (0.27, 0.31, 0.25, 1))
        for lateral, distance in ((-4.5, 100), (4.5, 175), (-4.0, 400), (4.0, 650)):
            self._add_background_target(lateral, distance)

    def _add_background_target(self, lateral: float, distance: float) -> None:
        loader = self.base.loader
        root = self.world_root.attachNewNode("background-target")
        root.setPos(lateral, distance, 0)
        add_box(loader, root, "post", (0, 0, 1.1), (0.07, 0.07, 1.1), (0.31, 0.24, 0.16, 1))
        add_box(loader, root, "board", (0, 0, 2.3), (0.85, 0.06, 0.85), (0.78, 0.75, 0.64, 1))
        add_sphere(loader, root, "plate", (0, -0.08, 2.3), (0.45, 0.05, 0.45), (0.28, 0.35, 0.38, 1))

    def _build_target(self) -> None:
        loader = self.base.loader
        self.target_root = self.world_root.attachNewNode("active-target")
        add_box(loader, self.target_root, "left-post", (-1.05, 0, 1.2), (0.07, 0.07, 1.2), (0.28, 0.20, 0.13, 1))
        add_box(loader, self.target_root, "right-post", (1.05, 0, 1.2), (0.07, 0.07, 1.2), (0.28, 0.20, 0.13, 1))
        add_box(loader, self.target_root, "board", (0, 0, 2.0), (1.4, 0.08, 1.4), (0.81, 0.78, 0.66, 1))
        self.target_plate = add_sphere(loader, self.target_root, "active-plate", (0, -0.11, 2.0), (0.75, 0.06, 0.75), (0.76, 0.20, 0.16, 1))
        self.target_center = add_sphere(loader, self.target_root, "target-center", (0, -0.18, 2.0), (0.18, 0.035, 0.18), (0.94, 0.86, 0.55, 1))
        self.set_target_distance(self.target_distance_m)

    def _build_view_model(self) -> None:
        loader = self.base.loader
        self.view_model = self.base.camera.attachNewNode("first-person-rig")
        self.view_model.setDepthTest(False)
        self.view_model.setDepthWrite(False)
        self.view_model.setBin("fixed", 50)
        rifle = (0.16, 0.20, 0.23, 1)
        metal = (0.08, 0.10, 0.11, 1)
        wood = (0.27, 0.18, 0.11, 1)
        glove = (0.12, 0.17, 0.14, 1)
        sleeve = (0.20, 0.25, 0.18, 1)
        add_box(loader, self.view_model, "stock", (0.24, 0.86, -0.30), (0.09, 0.22, 0.075), wood)
        add_box(loader, self.view_model, "receiver", (0.24, 1.16, -0.24), (0.075, 0.25, 0.06), rifle)
        add_box(loader, self.view_model, "barrel", (0.24, 1.68, -0.20), (0.016, 0.34, 0.016), metal)
        add_box(loader, self.view_model, "muzzle", (0.24, 2.04, -0.20), (0.03, 0.045, 0.03), metal)
        add_box(loader, self.view_model, "scope", (0.24, 1.18, -0.14), (0.04, 0.17, 0.035), metal)
        add_box(loader, self.view_model, "scope-front", (0.24, 1.37, -0.14), (0.06, 0.04, 0.06), metal)
        add_box(loader, self.view_model, "scope-rear", (0.24, 0.99, -0.14), (0.055, 0.04, 0.055), metal)
        add_box(loader, self.view_model, "left-arm", (-0.08, 0.92, -0.39), (0.065, 0.25, 0.06), sleeve).setH(-10)
        add_box(loader, self.view_model, "left-glove", (0.15, 1.37, -0.29), (0.07, 0.10, 0.06), glove)
        add_box(loader, self.view_model, "right-arm", (0.39, 0.70, -0.40), (0.07, 0.24, 0.065), sleeve).setH(10)
        add_box(loader, self.view_model, "right-glove", (0.32, 1.00, -0.31), (0.07, 0.095, 0.065), glove)
        self.muzzle_flash = add_sphere(loader, self.view_model, "muzzle-flash", (0.24, 2.13, -0.20), (0.055, 0.08, 0.055), (1.0, 0.68, 0.18, 1))
        self.muzzle_flash.hide()
        flash_light = PointLight("muzzle-light")
        flash_light.setColor((1.0, 0.52, 0.12, 1.0))
        flash_light.setAttenuation((1.0, 0.0, 2.0))
        self.flash_light_node = self.view_model.attachNewNode(flash_light)
        self.flash_light_node.setPos(0.24, 2.10, -0.20)
        self.flash_light_node.hide()

    def _build_overlay(self) -> None:
        aspect_ratio = max(self.base.getAspectRatio(), 0.5)
        texture_width = 640
        texture_height = max(320, min(640, round(texture_width / aspect_ratio)))
        self.scope_overlay = add_fullscreen_texture(
            self.base, make_scope_texture(texture_width, texture_height)
        )
        self.scope_overlay.hide()
        self.crosshair = OnscreenText(
            text="+",
            pos=(0, -0.02),
            scale=0.045,
            fg=(0.76, 0.88, 0.80, 0.85),
            align=TextNode.ACenter,
            mayChange=False,
        )
        self.instructions = OnscreenText(
            text="CLICK VIEW TO AIM   •   ESC RELEASE   •   LMB FIRE   •   RMB SCOPE",
            pos=(0, -0.92),
            scale=0.038,
            fg=(0.86, 0.91, 0.93, 0.9),
            shadow=(0, 0, 0, 0.7),
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
        self.instructions.setText("AIM ACTIVE   •   ESC RELEASE   •   LMB FIRE   •   RMB SCOPE")
        properties = self.base.win.getProperties()
        self.base.win.movePointer(0, properties.getXSize() // 2, properties.getYSize() // 2)

    def release_mouse(self) -> None:
        self.mouse_captured = False
        self.instructions.setText("CLICK VIEW TO AIM   •   ESC RELEASE   •   LMB FIRE   •   RMB SCOPE")

    def toggle_scope(self) -> None:
        self.scope_enabled = not self.scope_enabled
        if self.scope_enabled:
            self.scope_overlay.show()
        self.on_scope_changed(self.scope_enabled)

    def set_target_distance(self, distance_m: float) -> None:
        self.target_distance_m = distance_m
        self.target_root.setPos(0.0, distance_m, 0.0)

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
        self.recoil_started_s = self.shot_started_s
        self.muzzle_flash.show()
        self.flash_light_node.show()
        self.pending_score = score_target_plane(result.samples, result.scenario.target_distance_m)
        return self.pending_score

    def _replace_trajectory(self, result: ShotResult) -> None:
        if self.trajectory_node is not None:
            self.trajectory_node.removeNode()
        lines = LineSegs("trajectory-path")
        lines.setThickness(2.0)
        lines.setColor(0.22, 0.88, 1.0, 0.72)
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
            self.tracer = add_sphere(self.base.loader, self.effects_root, "tracer", (0, 0, 0), (0.06, 0.06, 0.06), (1.0, 0.75, 0.18, 1))
            self.impact_marker = add_sphere(self.base.loader, self.effects_root, "impact", (0, 0, 0), (0.18, 0.18, 0.18), (1.0, 0.20, 0.12, 1))
        self.tracer.show()
        self.impact_marker.hide()

    def reset_target_color(self) -> None:
        self.target_plate.setColor(0.76, 0.20, 0.16, 1)
        self.target_center.setColor(0.94, 0.86, 0.55, 1)

    def reset(self) -> None:
        self.release_mouse()
        self.scope_enabled = False
        self.scope_blend = 0.0
        self.scope_overlay.hide()
        self.crosshair.show()
        self.base.camLens.setFov(72.0)
        self.reset_target_color()
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
        sensitivity = 0.018 if self.scope_enabled else 0.055
        self.azimuth_deg = max(-50.0, min(50.0, self.azimuth_deg + delta_x * sensitivity))
        self.elevation_deg = max(-8.0, min(30.0, self.elevation_deg - delta_y * sensitivity))
        self.base.win.movePointer(0, center_x, center_y)
        aim = (self.elevation_deg, self.azimuth_deg)
        if aim != self._last_aim_emit:
            self._last_aim_emit = aim
            self.on_aim_changed(*aim)

    def _update_scope_and_rig(self, now: float) -> None:
        target = 1.0 if self.scope_enabled else 0.0
        speed = min(1.0, self.clock.getDt() * 7.5)
        self.scope_blend += (target - self.scope_blend) * speed
        self.base.camLens.setFov(72.0 + (14.0 - 72.0) * self.scope_blend)
        self.scope_overlay.setColorScale(1, 1, 1, self.scope_blend)
        if self.scope_blend < 0.01 and not self.scope_enabled:
            self.scope_overlay.hide()
        self.crosshair.setColorScale(1, 1, 1, 1.0 - self.scope_blend)
        if self.scope_blend > 0.82:
            self.view_model.hide()
            self.instructions.hide()
        else:
            self.view_model.show()
            self.instructions.show()
        breathing = math.sin(now * 1.6) * 0.006
        recoil = 0.0
        if self.recoil_started_s >= 0.0:
            recoil_age = now - self.recoil_started_s
            if recoil_age < 0.22:
                recoil = math.sin(recoil_age / 0.22 * math.pi) * 0.10
            else:
                self.recoil_started_s = -1.0
        self.view_model.setPos(
            -0.20 * self.scope_blend,
            0.42 - recoil,
            breathing + 0.07 * self.scope_blend,
        )
        self.base.camera.setHpr(-self.azimuth_deg, self.elevation_deg, 0.0)
        if self.recoil_started_s < 0.0 or now - self.shot_started_s > 0.08:
            self.muzzle_flash.hide()
            self.flash_light_node.hide()

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
                    self.target_plate.setColor(0.19, 0.75, 0.42, 1)
                elif self.pending_score.reached_target:
                    self.target_plate.setColor(0.92, 0.48, 0.12, 1)
                else:
                    self.target_plate.setColor(0.42, 0.48, 0.52, 1)
            self.shot_started_s = -1.0
            return
        index = bisect_left(self.shot_times, simulated_time)
        index = max(1, min(index, len(self.shot_times) - 1))
        previous = self.shot_result.samples[index - 1]
        current = self.shot_result.samples[index]
        delta = current.time_s - previous.time_s
        alpha = 0.0 if delta == 0.0 else (simulated_time - previous.time_s) / delta
        p0 = sample_to_panda(previous)
        p1 = sample_to_panda(current)
        self.tracer.setPos(*(p0[axis] + alpha * (p1[axis] - p0[axis]) for axis in range(3)))

    def save_screenshot(self, path: Path) -> bool:
        return bool(self.base.win.saveScreenshot(Filename.fromOsSpecific(str(path))))

    def destroy(self) -> None:
        self.base.taskMgr.remove("ballistics-range-update")
        self.base.ignoreAll()
        self.scope_overlay.removeNode()
        self.world_root.removeNode()
        self.effects_root.removeNode()
