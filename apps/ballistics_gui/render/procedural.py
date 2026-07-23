"""Small procedural building blocks for the self-contained range."""

from __future__ import annotations

import math

from panda3d.core import CardMaker, NodePath, PNMImage, Texture, TransparencyAttrib


def add_box(loader, parent: NodePath, name: str, position, scale, color) -> NodePath:
    model = loader.loadModel("models/box")
    model.setName(name)
    model.reparentTo(parent)
    model.clearTexture()
    model.setTextureOff(1)
    model.setColor(*color)
    model.setPos(*position)
    model.setScale(*scale)
    return model


def add_sphere(loader, parent: NodePath, name: str, position, scale, color) -> NodePath:
    model = loader.loadModel("models/misc/sphere")
    model.setName(name)
    model.reparentTo(parent)
    model.clearTexture()
    model.setTextureOff(1)
    model.setColor(*color)
    model.setPos(*position)
    model.setScale(*scale)
    return model


def add_card(parent: NodePath, name: str, frame, color) -> NodePath:
    maker = CardMaker(name)
    maker.setFrame(*frame)
    card = parent.attachNewNode(maker.generate())
    card.setColor(*color)
    card.setTwoSided(True)
    return card


def make_scope_texture(width: int = 640, height: int = 360) -> Texture:
    """Build a decorative scope mask and reticle without an external image asset."""

    image = PNMImage(width, height, 4)
    center_x = (width - 1) * 0.5
    center_y = (height - 1) * 0.5
    radius = min(width, height) * 0.46
    ring_inner = radius - 2.5
    reticle_color = (0.72, 0.86, 0.76)
    for y in range(height):
        dy = y - center_y
        for x in range(width):
            dx = x - center_x
            distance = math.hypot(dx, dy)
            alpha = 0.0
            red = green = blue = 0.0
            if distance > radius:
                alpha = 0.97
            elif distance >= ring_inner:
                red, green, blue = reticle_color
                alpha = 0.9
            elif (abs(dx) <= 0.7 and abs(dy) > 10.0) or (
                abs(dy) <= 0.7 and abs(dx) > 10.0
            ):
                red, green, blue = reticle_color
                alpha = 0.72
            elif 7.0 <= distance <= 8.5:
                red, green, blue = reticle_color
                alpha = 0.75
            image.setXelA(x, y, red, green, blue, alpha)
    texture = Texture("procedural-scope-overlay")
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
