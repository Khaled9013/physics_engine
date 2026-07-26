"""Imported first-person hands, precision rifle, optic, and shot animation.

The rig previously rendered with both shading and lighting disabled so it stayed legible
regardless of sun direction. That kept it readable but removed every cue that gives an
object form, which is why it read as flat cut-outs. It is now lit normally and additionally
carries its own camera-space key and rim lights, so scene lighting still reaches it while a
consistent baseline keeps it readable when the shooter faces away from the sun.

Depth testing is likewise enabled rather than disabled. The rig sits between 0.35 m and
1.6 m from the eye, inside the world's near range, so it occludes the range correctly and —
unlike before — occludes itself correctly too.
"""

from __future__ import annotations

import math

from direct.actor.Actor import Actor
from panda3d.core import DirectionalLight, PointLight

from .assets import VisualAssetPaths, load_model, require_asset
from .lighting import SHADOW_CASTER_MASK
from .materials import apply_flat_material, apply_texture_set
from .procedural import add_sphere

RIFLE_TEXCOORD_SET = "texcoord.0"

RIG_KEY_COLOR = (0.72, 0.74, 0.79, 1.0)
RIG_KEY_HPR = (-32.0, -26.0, 0.0)
RIG_RIM_COLOR = (0.22, 0.26, 0.32, 1.0)
RIG_RIM_HPR = (152.0, 18.0, 0.0)

# Rifle placement in camera space. The rig sits right of centre and below the eye line, far
# enough forward to clear the world near plane along its whole length.
RIFLE_SCALE = 0.80
RIFLE_POSITION = (0.118, 0.72, -0.150)
SCOPE_SCALE = 0.55
# Residual lift applied after the arms are fitted to the rifle, so the gloves close on the
# stock rather than floating just under it.
HANDS_VERTICAL_TRIM = -0.018
GLOVE_COLOR = (0.085, 0.095, 0.080, 1.0)
CUFF_COLOR = (0.065, 0.072, 0.062, 1.0)


class PrecisionViewModel:
    """Compose authored CC0 assets into one fictional first-person rig."""

    def __init__(self, base, assets: VisualAssetPaths) -> None:
        self.base = base
        self.assets = assets
        self.root = base.camera.attachNewNode("first-person-precision-rig")
        self.root.setBin("fixed", 50)
        self.root.hide(SHADOW_CASTER_MASK)
        self.recoil_started_s = -1.0
        self._build_rig_lighting()
        # The rifle is placed first so the hands can be fitted to it rather than to the
        # coarse proxy weapon they were authored around.
        self._build_rifle()
        self._build_hands()
        self._build_scope()
        self._build_muzzle_effect()

    def _build_rig_lighting(self) -> None:
        """Attach camera-space lights that apply only to the rig."""

        key = DirectionalLight("rig-key")
        key.setColor(RIG_KEY_COLOR)
        self.key_node = self.root.attachNewNode(key)
        self.key_node.setHpr(*RIG_KEY_HPR)
        self.root.setLight(self.key_node)

        rim = DirectionalLight("rig-rim")
        rim.setColor(RIG_RIM_COLOR)
        self.rim_node = self.root.attachNewNode(rim)
        self.rim_node.setHpr(*RIG_RIM_HPR)
        self.root.setLight(self.rim_node)

    def _build_hands(self) -> None:
        self.hands = Actor(str(require_asset(self.assets.hands_model)))
        if self.hands.isEmpty():
            raise RuntimeError(f"Panda3D could not load visual asset: {self.assets.hands_model}")
        self.hands.reparentTo(self.root)
        proxy_gun = self.hands.find("**/Gun")
        if proxy_gun.isEmpty():
            raise RuntimeError("FPS hands asset is missing its separable proxy gun node")
        self._fit_hands_to_rifle(proxy_gun)
        proxy_gun.hide()
        for node_name, color, roughness in (
            ("Hand1", GLOVE_COLOR, 0.74),
            ("Hand2", CUFF_COLOR, 0.78),
        ):
            hand = self.hands.find(f"**/{node_name}")
            if hand.isEmpty():
                raise RuntimeError(f"FPS hands asset is missing {node_name}")
            # The bundled texture is a flat low-resolution atlas that reads as noise at
            # viewmodel scale, so the gloves are shaded by material alone.
            hand.setTextureOff(1)
            apply_flat_material(hand, f"{node_name}-glove", color, roughness=roughness)
        if "fire" in self.hands.getAnimNames():
            self.hands.pose("fire", 0)

    def _fit_hands_to_rifle(self, proxy_gun) -> None:
        """Scale and position the arms so their grip lands on the real rifle.

        The arms were authored holding the coarse proxy weapon bundled with the asset, which
        is a different length and sits at a different height than the detailed rifle. Fitting
        the proxy's bounding box onto the rifle's keeps the grip aligned no matter how the
        rifle's own scale or position is later retuned.
        """

        gun_low, gun_high = proxy_gun.getTightBounds(self.hands)
        rifle_low, rifle_high = self.rifle.getTightBounds(self.root)

        gun_length = gun_high.y - gun_low.y
        if gun_length <= 0.0:
            raise RuntimeError("FPS hands proxy gun has no measurable length")
        scale = (rifle_high.y - rifle_low.y) / gun_length
        self.hands.setScale(scale)

        # All three axes align centre-to-centre. The proxy weapon is a blocky mass with
        # proportions unlike the real rifle, so its bounding box cannot locate the grip
        # exactly; the vertical result is corrected by an explicit trim tuned against a
        # rendered frame rather than derived from the box.
        self.hands.setPos(
            (rifle_low.x + rifle_high.x) * 0.5 - (gun_low.x + gun_high.x) * 0.5 * scale,
            (rifle_low.y + rifle_high.y) * 0.5 - (gun_low.y + gun_high.y) * 0.5 * scale,
            (rifle_low.z + rifle_high.z) * 0.5
            - (gun_low.z + gun_high.z) * 0.5 * scale
            + HANDS_VERTICAL_TRIM,
        )

    def _build_rifle(self) -> None:
        self.rifle = load_model(self.base.loader, self.assets.rifle_model)
        self.rifle.reparentTo(self.root)
        self.rifle.setPos(*RIFLE_POSITION)
        self.rifle.setH(180.0)
        self.rifle.setScale(RIFLE_SCALE)
        # The committed normal and ORM maps were vendored with the model but never bound,
        # so the receiver, stock, and barrel previously carried no surface detail at all.
        apply_texture_set(
            self.base.loader,
            self.rifle,
            "rifle",
            self.assets.rifle,
            texcoord_name=RIFLE_TEXCOORD_SET,
        )
        self.rifle.setColor(1.0, 1.0, 1.0, 1.0)

    def _build_scope(self) -> None:
        rifle_low, rifle_high = self.rifle.getTightBounds(self.root)
        centre_x = (rifle_low.x + rifle_high.x) * 0.5
        self.scope = load_model(self.base.loader, self.assets.scope_model)
        self.scope.reparentTo(self.root)
        self.scope.setH(180.0)
        self.scope.setScale(SCOPE_SCALE)
        # Seat the optic on the receiver: centred laterally, just above the rifle's top
        # surface, and set back from the muzzle where a rail would be.
        scope_low, scope_high = self.scope.getTightBounds(self.root)
        self.scope.setPos(
            centre_x - (scope_low.x + scope_high.x) * 0.5,
            rifle_low.y + (rifle_high.y - rifle_low.y) * 0.56
            - (scope_low.y + scope_high.y) * 0.5,
            rifle_high.z - scope_low.z + 0.004,
        )
        apply_flat_material(
            self.scope,
            "scope-metal",
            (0.045, 0.048, 0.052, 1.0),
            roughness=0.38,
            metallic=0.80,
        )
        scope_low, scope_high = self.scope.getTightBounds(self.root)
        self.scope_glass = add_sphere(
            self.base.loader,
            self.root,
            "scope-eyepiece-glass",
            (
                centre_x,
                scope_low.y + 0.006,
                (scope_low.z + scope_high.z) * 0.5,
            ),
            (0.017, 0.004, 0.017),
            (0.020, 0.075, 0.062, 1.0),
        )
        apply_flat_material(
            self.scope_glass,
            "scope-glass-material",
            (0.020, 0.075, 0.062, 1.0),
            roughness=0.06,
            metallic=0.30,
        )

    def _build_muzzle_effect(self) -> None:
        rifle_low, rifle_high = self.rifle.getTightBounds(self.root)
        muzzle = (
            (rifle_low.x + rifle_high.x) * 0.5,
            rifle_high.y + 0.02,
            rifle_low.z + (rifle_high.z - rifle_low.z) * 0.62,
        )
        self.muzzle_flash = add_sphere(
            self.base.loader,
            self.root,
            "muzzle-flash",
            muzzle,
            (0.048, 0.125, 0.048),
            (1.0, 0.46, 0.09, 1.0),
        )
        apply_flat_material(
            self.muzzle_flash,
            "muzzle-emissive",
            (1.0, 0.42, 0.06, 1.0),
            roughness=0.30,
            emission=(1.0, 0.34, 0.05, 1.0),
        )
        self.muzzle_flash.hide()
        flash_light = PointLight("muzzle-light")
        flash_light.setColor((1.9, 0.72, 0.16, 1.0))
        flash_light.setAttenuation((1.0, 0.0, 2.4))
        self.flash_light_node = self.root.attachNewNode(flash_light)
        self.flash_light_node.setPos(*muzzle)
        self.flash_light_node.hide()
        self.base.render.setLight(self.flash_light_node)

    def fire(self, now_s: float) -> None:
        self.recoil_started_s = now_s
        if "fire" in self.hands.getAnimNames():
            self.hands.play("fire")
        self.muzzle_flash.show()
        self.flash_light_node.show()

    def update(self, now_s: float, _delta_s: float, scope_blend: float) -> None:
        # Breathing and sway shrink as the shooter settles behind the optic.
        settle = 1.0 - 0.62 * scope_blend
        sway_x = math.sin(now_s * 0.75) * 0.0045 * settle
        sway_r = math.sin(now_s * 0.52) * 0.22 * settle
        breathing = math.sin(now_s * 1.45) * 0.005 * settle
        recoil = 0.0
        recoil_pitch = 0.0
        if self.recoil_started_s >= 0.0:
            recoil_age = now_s - self.recoil_started_s
            if recoil_age < 0.30:
                # A fast rise and slower settle reads more like recoil than a symmetric pulse.
                phase = recoil_age / 0.30
                pulse = math.sin(phase * math.pi) * (1.0 - phase) ** 0.35
                recoil = pulse * 0.14
                recoil_pitch = pulse * 3.4
            else:
                self.recoil_started_s = -1.0

        self.root.setPos(
            sway_x - 0.128 * scope_blend,
            -recoil + 0.050 * scope_blend,
            breathing - 0.052 + 0.128 * scope_blend,
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
