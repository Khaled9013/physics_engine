"""Reusable procedural geometry and generated presentation textures.

Every texture generated here is produced from a fixed seed, so two launches of the
application build byte-identical images. Nothing in this module reads the filesystem or the
network; generated content exists for surfaces that have no committed texture set, such as
foliage cut-outs, the target face, and the optic overlay.
"""

from __future__ import annotations

import math

from panda3d.core import (
    CardMaker,
    NodePath,
    PNMImage,
    SamplerState,
    Texture,
    TransparencyAttrib,
)

from .materials import apply_flat_material


class _Deterministic:
    """A small fixed-seed generator, so generated art never varies between launches."""

    def __init__(self, seed: int) -> None:
        self._state = seed & 0xFFFFFFFF

    def next_float(self) -> float:
        self._state = (1103515245 * self._state + 12345) & 0x7FFFFFFF
        return self._state / 0x7FFFFFFF

    def between(self, low: float, high: float) -> float:
        return low + (high - low) * self.next_float()


def add_box(loader, parent: NodePath, name: str, position, scale, color) -> NodePath:
    model = loader.loadModel("models/box")
    model.setName(name)
    model.reparentTo(parent)
    model.clearTexture()
    model.setTextureOff(1)
    apply_flat_material(model, f"{name}-material", color, roughness=0.86)
    # Panda3D's unit box spans 0..1, so centre it on the requested position.
    model.setScale(*(component * 2.0 for component in scale))
    model.setPos(
        position[0] - scale[0],
        position[1] - scale[1],
        position[2] - scale[2],
    )
    return model


def add_sphere(loader, parent: NodePath, name: str, position, scale, color) -> NodePath:
    model = loader.loadModel("models/misc/sphere")
    model.setName(name)
    model.reparentTo(parent)
    model.clearTexture()
    model.setTextureOff(1)
    apply_flat_material(model, f"{name}-material", color, roughness=0.84)
    model.setPos(*position)
    model.setScale(*scale)
    return model


def add_card(parent: NodePath, name: str, frame, color) -> NodePath:
    maker = CardMaker(name)
    maker.setFrame(*frame)
    card = parent.attachNewNode(maker.generate())
    apply_flat_material(card, f"{name}-material", color, roughness=0.94)
    card.setTwoSided(True)
    return card


def add_textured_card(
    parent: NodePath,
    name: str,
    frame,
    texture: Texture,
    texture_scale: tuple[float, float] | None = None,
) -> NodePath:
    """Create a two-sided card carrying one generated or committed texture."""

    maker = CardMaker(name)
    maker.setFrame(*frame)
    card = parent.attachNewNode(maker.generate())
    card.setTexture(texture, 1)
    card.setTwoSided(True)
    if texture_scale is not None:
        card.setTexScale(card.findTextureStage("*"), *texture_scale)
    return card


def add_cross_billboard(
    parent: NodePath,
    name: str,
    texture: Texture,
    width: float,
    height: float,
) -> NodePath:
    """Build two intersecting alpha cards, the standard cheap stand-in for foliage."""

    root = parent.attachNewNode(name)
    for index, heading in enumerate((0.0, 90.0)):
        maker = CardMaker(f"{name}-blade-{index}")
        maker.setFrame(-width * 0.5, width * 0.5, 0.0, height)
        card = root.attachNewNode(maker.generate())
        card.setH(heading)
        card.setTwoSided(True)
        card.setTexture(texture, 1)
        card.setTransparency(TransparencyAttrib.MBinary)
    return root


def _alpha_texture(name: str, image: PNMImage) -> Texture:
    texture = Texture(name)
    texture.load(image)
    texture.setMinfilter(SamplerState.FT_linear_mipmap_linear)
    texture.setMagfilter(SamplerState.FT_linear)
    texture.setWrapU(SamplerState.WM_clamp)
    texture.setWrapV(SamplerState.WM_clamp)
    return texture


def make_grass_tuft_texture(size: int = 128) -> Texture:
    """Generate a cut-out clump of grass blades for near-field ground clutter."""

    image = PNMImage(size, size, 4)
    image.fill(0.0, 0.0, 0.0)
    image.alphaFill(0.0)
    generator = _Deterministic(0x5EED_0001)
    for _ in range(26):
        base_x = generator.between(0.16, 0.84) * size
        blade_height = generator.between(0.45, 0.95) * size
        lean = generator.between(-0.30, 0.30) * size
        half_width = generator.between(0.008, 0.020) * size
        shade = generator.between(0.55, 1.0)
        for step in range(int(blade_height)):
            t = step / max(1.0, blade_height)
            x_centre = base_x + lean * t * t
            y = size - 1 - step
            taper = half_width * (1.0 - t * 0.85)
            green = (0.16 + 0.30 * (0.35 + 0.65 * t)) * shade
            red = (0.09 + 0.13 * t) * shade
            blue = (0.05 + 0.07 * t) * shade
            for offset in range(-int(taper) - 1, int(taper) + 2):
                x = int(x_centre) + offset
                if 0 <= x < size and 0 <= y < size:
                    if abs(offset) <= taper:
                        image.setXelA(x, y, red, green, blue, 1.0)
    return _alpha_texture("grass-tuft", image)


def make_canopy_texture(size: int = 256) -> Texture:
    """Generate a cut-out foliage mass for distant tree billboards."""

    image = PNMImage(size, size, 4)
    image.fill(0.0, 0.0, 0.0)
    image.alphaFill(0.0)
    generator = _Deterministic(0x5EED_0002)
    centre = size * 0.5
    for _ in range(120):
        blob_x = generator.between(0.12, 0.88) * size
        blob_y = generator.between(0.06, 0.78) * size
        radius = generator.between(0.045, 0.115) * size
        shade = generator.between(0.45, 1.0)
        # Fade clusters that stray from the centre so the silhouette stays organic.
        falloff = 1.0 - min(1.0, math.hypot(blob_x - centre, blob_y - centre * 0.85) / (size * 0.52))
        if falloff <= 0.05:
            continue
        for y in range(max(0, int(blob_y - radius)), min(size, int(blob_y + radius) + 1)):
            for x in range(max(0, int(blob_x - radius)), min(size, int(blob_x + radius) + 1)):
                distance = math.hypot(x - blob_x, y - blob_y)
                if distance > radius:
                    continue
                depth = 1.0 - (y / size) * 0.45
                image.setXelA(
                    x,
                    y,
                    (0.075 + 0.070 * shade) * depth,
                    (0.115 + 0.130 * shade) * depth,
                    (0.055 + 0.055 * shade) * depth,
                    1.0,
                )
    return _alpha_texture("tree-canopy", image)


def make_target_face_texture(size: int = 512) -> Texture:
    """Generate the practice target's printed face.

    Flat printed rings on one card replace the previous stack of flattened spheres, which
    produced visible intersection artefacts and could not be read at range.
    """

    image = PNMImage(size, size, 4)
    centre = (size - 1) * 0.5
    max_radius = size * 0.46
    rings = (
        (0.20, (0.94, 0.80, 0.22)),
        (0.36, (0.80, 0.22, 0.16)),
        (0.54, (0.90, 0.89, 0.85)),
        (0.74, (0.14, 0.16, 0.17)),
        (1.00, (0.86, 0.85, 0.80)),
    )
    generator = _Deterministic(0x5EED_0003)
    weathering = [generator.between(0.93, 1.0) for _ in range(64)]
    for y in range(size):
        for x in range(size):
            distance = math.hypot(x - centre, y - centre) / max_radius
            if distance > 1.02:
                image.setXelA(x, y, 0.0, 0.0, 0.0, 0.0)
                continue
            colour = rings[-1][1]
            for edge, ring_colour in rings:
                if distance <= edge:
                    colour = ring_colour
                    break
            stain = weathering[(x // 8 + y // 8) % len(weathering)]
            # A thin dark rule on each ring boundary keeps the divisions legible at range.
            boundary = any(abs(distance - edge) < 0.012 for edge, _ in rings[:-1])
            if boundary:
                colour = (colour[0] * 0.45, colour[1] * 0.45, colour[2] * 0.45)
            cross = (abs(x - centre) < 1.2 or abs(y - centre) < 1.2) and distance < 0.16
            if cross:
                colour = (0.10, 0.10, 0.11)
            image.setXelA(x, y, colour[0] * stain, colour[1] * stain, colour[2] * stain, 1.0)
    return _alpha_texture("target-face", image)


def make_impact_decal_texture(size: int = 64) -> Texture:
    """Generate a bullet-hole decal with a soft dark rim."""

    image = PNMImage(size, size, 4)
    centre = (size - 1) * 0.5
    radius = size * 0.42
    for y in range(size):
        for x in range(size):
            distance = math.hypot(x - centre, y - centre) / radius
            if distance > 1.0:
                image.setXelA(x, y, 0.0, 0.0, 0.0, 0.0)
            elif distance < 0.52:
                image.setXelA(x, y, 0.03, 0.03, 0.035, 1.0)
            else:
                fade = 1.0 - (distance - 0.52) / 0.48
                image.setXelA(x, y, 0.16, 0.15, 0.14, fade * 0.85)
    return _alpha_texture("impact-decal", image)


def make_scope_texture(width: int = 1024, height: int = 576) -> Texture:
    """Build the optic overlay: eye-box vignette, lens tint, and a fine reticle.

    The overlay is decorative. It carries no real optic calibration, and its marks are not
    tied to the solver's drop or drift output.
    """

    image = PNMImage(width, height, 4)
    centre_x = (width - 1) * 0.5
    centre_y = (height - 1) * 0.5
    radius = min(width, height) * 0.472
    rim_inner = radius - max(2.0, radius * 0.016)
    reticle = (0.055, 0.075, 0.065)
    tick_marks = tuple(radius * fraction for fraction in (0.16, 0.32, 0.48, 0.64, 0.80))
    for y in range(height):
        dy = y - centre_y
        for x in range(width):
            dx = x - centre_x
            distance = math.hypot(dx, dy)
            if distance > radius:
                image.setXelA(x, y, 0.0, 0.0, 0.0, 1.0)
                continue
            if distance >= rim_inner:
                image.setXelA(x, y, 0.05, 0.055, 0.055, 1.0)
                continue

            # Eye-box shading darkens smoothly from roughly two-thirds out to the rim.
            edge = max(0.0, (distance / radius - 0.62) / 0.38)
            alpha = edge * edge * 0.72
            red, green, blue = (0.012, 0.022, 0.020)

            hair_half_width = max(0.55, radius * 0.0016)
            inner_gap = radius * 0.035
            horizontal = abs(dy) <= hair_half_width and inner_gap < abs(dx) < rim_inner
            vertical = abs(dx) <= hair_half_width and inner_gap < abs(dy) < rim_inner
            centre_dot = distance <= max(1.1, radius * 0.006)

            tick = False
            tick_length = radius * 0.030
            if abs(dy) <= tick_length:
                tick = any(abs(abs(dx) - mark) <= hair_half_width for mark in tick_marks)
            if not tick and abs(dx) <= tick_length:
                tick = any(abs(abs(dy) - mark) <= hair_half_width for mark in tick_marks)

            # The lower vertical post thickens below centre, a common holdover convention.
            post = (
                abs(dx) <= hair_half_width * 2.4
                and dy > radius * 0.52
                and abs(dy) < rim_inner
            )

            if horizontal or vertical or centre_dot or tick or post:
                red, green, blue = reticle
                alpha = max(alpha, 0.94 if (centre_dot or tick or post) else 0.86)

            image.setXelA(x, y, red, green, blue, alpha)

    texture = Texture("precision-scope-overlay")
    texture.load(image)
    texture.setMinfilter(Texture.FTLinear)
    texture.setMagfilter(Texture.FTLinear)
    texture.setWrapU(SamplerState.WM_clamp)
    texture.setWrapV(SamplerState.WM_clamp)
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
