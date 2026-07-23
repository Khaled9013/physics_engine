"""Imported first-person hands, precision rifle, optic, and shot animation."""

from __future__ import annotations

import math

from direct.actor.Actor import Actor
from panda3d.core import InternalName, PointLight, TextureStage

from .assets import VisualAssetPaths, load_model, load_texture, require_asset
from .procedural import add_sphere, apply_material


class PrecisionViewModel:
    """Compose authored CC0 assets into one fictional first-person rig."""

    def __init__(self, base, assets: VisualAssetPaths) -> None:
        self.base = base
        self.assets = assets
        self.root = base.camera.attachNewNode("first-person-precision-rig")
        self.root.setDepthTest(False)
        self.root.setDepthWrite(False)
        self.root.setBin("fixed", 50)
        # Camera-space presentation is intentionally unlit to remain readable
        # independently of world shadow direction and exposure.
        self.root.setShaderOff(100)
        self.root.setLightOff(100)
        self.recoil_started_s = -1.0
        self._build_hands()
        self._build_rifle()
        self._build_scope()
        self._build_muzzle_effect()

    def _build_hands(self) -> None:
        self.hands = Actor(str(require_asset(self.assets.hands_model)))
        if self.hands.isEmpty():
            raise RuntimeError(f"Panda3D could not load visual asset: {self.assets.hands_model}")
        self.hands.reparentTo(self.root)
        self.hands.setPos(0.15, 0.74, -0.31)
        self.hands.setScale(0.09)
        proxy_gun = self.hands.find("**/Gun")
        if proxy_gun.isEmpty():
            raise RuntimeError("FPS hands asset is missing its separable proxy gun node")
        proxy_gun.hide()
        for node_name, color in (
            ("Hand1", (0.16, 0.21, 0.17, 1.0)),
            ("Hand2", (0.14, 0.19, 0.15, 1.0)),
        ):
            hand = self.hands.find(f"**/{node_name}")
            if hand.isEmpty():
                raise RuntimeError(f"FPS hands asset is missing {node_name}")
            apply_material(hand, f"{node_name}-glove", color, roughness=0.92)
        if "fire" in self.hands.getAnimNames():
            self.hands.pose("fire", 0)

    def _build_rifle(self) -> None:
        self.rifle = load_model(self.base.loader, self.assets.rifle_model)
        self.rifle.reparentTo(self.root)
        self.rifle.setPos(0.15, 0.74, -0.28)
        self.rifle.setH(180.0)
        self.rifle.setScale(0.66)

        base_color = load_texture(
            self.base.loader, self.assets.rifle_base_color
        )
        base_stage = TextureStage("rifle-base-color")
        base_stage.setMode(TextureStage.MModulate)
        base_stage.setTexcoordName(InternalName.make("texcoord.0"))
        self.rifle.setTexture(base_stage, base_color, 1)
        self.rifle.setColor(1.0, 1.0, 1.0, 1.0)

    def _build_scope(self) -> None:
        self.scope = load_model(self.base.loader, self.assets.scope_model)
        self.scope.reparentTo(self.root)
        self.scope.setPos(0.15, 0.80, -0.212)
        self.scope.setH(180.0)
        self.scope.setScale(0.62)
        apply_material(
            self.scope,
            "scope-metal",
            (0.11, 0.13, 0.14, 1.0),
            roughness=0.30,
            metallic=0.78,
        )
        self.scope_glass = add_sphere(
            self.base.loader,
            self.root,
            "scope-eyepiece-glass",
            (0.15, 0.61, -0.178),
            (0.015, 0.005, 0.015),
            (0.018, 0.065, 0.058, 1.0),
        )
        apply_material(
            self.scope_glass,
            "scope-glass-material",
            (0.018, 0.065, 0.058, 1.0),
            roughness=0.12,
            metallic=0.55,
        )

    def _build_muzzle_effect(self) -> None:
        self.muzzle_flash = add_sphere(
            self.base.loader,
            self.root,
            "muzzle-flash",
            (0.15, 1.24, -0.25),
            (0.055, 0.14, 0.055),
            (1.0, 0.42, 0.06, 1.0),
        )
        apply_material(
            self.muzzle_flash,
            "muzzle-emissive",
            (1.0, 0.38, 0.04, 1.0),
            roughness=0.25,
            emission=(1.0, 0.20, 0.015, 1.0),
        )
        self.muzzle_flash.hide()
        flash_light = PointLight("muzzle-light")
        flash_light.setColor((1.0, 0.35, 0.055, 1.0))
        flash_light.setAttenuation((1.0, 0.0, 3.5))
        self.flash_light_node = self.root.attachNewNode(flash_light)
        self.flash_light_node.setPos(0.15, 1.23, -0.25)
        self.flash_light_node.hide()
        self.base.render.setLight(self.flash_light_node)

    def fire(self, now_s: float) -> None:
        self.recoil_started_s = now_s
        if "fire" in self.hands.getAnimNames():
            self.hands.play("fire")
        self.muzzle_flash.show()
        self.flash_light_node.show()

    def update(self, now_s: float, _delta_s: float, scope_blend: float) -> None:
        sway_x = math.sin(now_s * 0.75) * 0.0045
        sway_r = math.sin(now_s * 0.52) * 0.22
        breathing = math.sin(now_s * 1.45) * 0.005
        recoil = 0.0
        recoil_pitch = 0.0
        if self.recoil_started_s >= 0.0:
            recoil_age = now_s - self.recoil_started_s
            if recoil_age < 0.24:
                pulse = math.sin(recoil_age / 0.24 * math.pi)
                recoil = pulse * 0.13
                recoil_pitch = pulse * 2.8
            else:
                self.recoil_started_s = -1.0

        self.root.setPos(
            sway_x - 0.145 * scope_blend,
            -recoil + 0.055 * scope_blend,
            breathing - 0.055 + 0.135 * scope_blend,
        )
        self.root.setHpr(sway_r, -recoil_pitch, -sway_r * 0.45)
        if self.recoil_started_s < 0.0 or now_s - self.recoil_started_s > 0.075:
            self.muzzle_flash.hide()
            self.flash_light_node.hide()

    def set_visible(self, visible: bool) -> None:
        if visible:
            self.root.show()
        else:
            self.root.hide()

    def reset(self) -> None:
        self.recoil_started_s = -1.0
        self.muzzle_flash.hide()
        self.flash_light_node.hide()
        self.root.setPos(0.0, 0.0, 0.0)
        self.root.setHpr(0.0, 0.0, 0.0)
        self.root.show()
        if "fire" in self.hands.getAnimNames():
            self.hands.pose("fire", 0)

    def destroy(self) -> None:
        self.base.render.clearLight(self.flash_light_node)
        self.hands.cleanup()
        self.root.removeNode()
