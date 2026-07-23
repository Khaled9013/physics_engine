"""Reusable procedural geometry and presentation helpers."""

from __future__ import annotations

import math

from panda3d.core import (
    CardMaker,
    Material,
    NodePath,
    PNMImage,
    Texture,
    TextureStage,
    TransparencyAttrib,
)


def apply_material(
    node: NodePath,
    name: str,
    color,
    *,
    roughness: float = 0.8,
    metallic: float = 0.0,
    emission=None,
) -> Material:
    """Apply a compact PBR-friendly material to one node hierarchy."""

    material = Material(name)
    material.setBaseColor(color)
    material.setRoughness(roughness)
    material.setMetallic(metallic)
    if emission is not None:
        material.setEmission(emission)
    node.setMaterial(material, 1)
    node.setColor(*color)
    return material


def add_box(loader, parent: NodePath, name: str, position, scale, color) -> NodePath:
    model = loader.loadModel("models/box")
    model.setName(name)
    model.reparentTo(parent)
    model.clearTexture()
    model.setTextureOff(1)
    apply_material(model, f"{name}-material", color, roughness=0.88)
    model.setPos(*position)
    model.setScale(*scale)
    return model


def add_sphere(loader, parent: NodePath, name: str, position, scale, color) -> NodePath:
    model = loader.loadModel("models/misc/sphere")
    model.setName(name)
    model.reparentTo(parent)
    model.clearTexture()
    model.setTextureOff(1)
    apply_material(model, f"{name}-material", color, roughness=0.82)
    model.setPos(*position)
    model.setScale(*scale)
    return model


def add_card(parent: NodePath, name: str, frame, color) -> NodePath:
    maker = CardMaker(name)
    maker.setFrame(*frame)
    card = parent.attachNewNode(maker.generate())
    apply_material(card, f"{name}-material", color, roughness=0.94)
    card.setTwoSided(True)
    return card


def add_textured_card(
    parent: NodePath,
    name: str,
    frame,
    texture: Texture,
    texture_scale: tuple[float, float],
) -> NodePath:
    """Create a two-sided card with repeatable UV scale."""

    card = add_card(parent, name, frame, (1.0, 1.0, 1.0, 1.0))
    stage = TextureStage.getDefault()
    card.setTexture(stage, texture, 1)
    card.setTexScale(stage, *texture_scale)
    return card


def make_scope_texture(width: int = 960, height: int = 540) -> Texture:
    """Build a high-resolution vignette and precision reticle."""

    image = PNMImage(width, height, 4)
    center_x = (width - 1) * 0.5
    center_y = (height - 1) * 0.5
    radius = min(width, height) * 0.475
    ring_inner = radius - 3.0
    reticle = (0.60, 0.76, 0.66)
    for y in range(height):
        dy = y - center_y
        for x in range(width):
            dx = x - center_x
            distance = math.hypot(dx, dy)
            red = green = blue = 0.0
            alpha = 0.0
            if distance > radius:
                alpha = 0.985
            elif distance >= ring_inner:
                red, green, blue = (0.13, 0.18, 0.17)
                alpha = 0.96
            else:
                edge = max(0.0, (distance / radius - 0.70) / 0.30)
                red, green, blue = (0.015, 0.04, 0.035)
                alpha = edge * edge * 0.58

                horizontal = abs(dy) <= 0.65 and 13.0 < abs(dx) < radius - 18.0
                vertical = abs(dx) <= 0.65 and 13.0 < abs(dy) < radius - 18.0
                center_ring = 7.2 <= distance <= 8.7
                tick = False
                if abs(dy) <= 4.0:
                    tick = any(abs(abs(dx) - mark) <= 0.75 for mark in (45, 90, 135, 180))
                if abs(dx) <= 4.0:
                    tick = tick or any(
                        abs(abs(dy) - mark) <= 0.75 for mark in (45, 90, 135, 180)
                    )
                if horizontal or vertical or center_ring or tick:
                    red, green, blue = reticle
                    alpha = max(alpha, 0.82 if tick else 0.72)

            image.setXelA(x, y, red, green, blue, alpha)

    texture = Texture("precision-scope-overlay")
    texture.load(image)
    texture.setMinfilter(Texture.FTLinear)
    texture.setMagfilter(Texture.FTLinear)
    return texture


def add_fullscreen_texture(base, texture: Texture) -> NodePath:
    maker = CardMaker("scope-overlay-card")
    maker.setFrameFullscreenQuad()
    card = base.render2d.attachNewNode(maker.generate())
    card.setTexture(texture)
    card.setTransparency(TransparencyAttrib.MAlpha)
    card.setBin("fixed", 100)
    card.setDepthTest(False)
    card.setDepthWrite(False)
    return card
