"""Physically-based material construction from committed texture sets.

`simplepbr` selects its shader samplers by texture-stage mode rather than by stage name:
``MModulate`` feeds base colour, ``MNormal`` feeds the tangent-space normal map, and
``MSelector`` feeds the packed occlusion/roughness/metalness map. The shader then multiplies
the sampled roughness and metalness by the corresponding `Material` scalars, so a material
backed by an ORM map must leave those scalars at 1.0 or the map is silently attenuated.
"""

from __future__ import annotations

from panda3d.core import InternalName, Material, NodePath, TextureStage

from .assets import TextureSet, load_texture


def _stage(name: str, mode, texcoord_name: str | None) -> TextureStage:
    stage = TextureStage(name)
    stage.setMode(mode)
    if texcoord_name is not None:
        stage.setTexcoordName(InternalName.make(texcoord_name))
    return stage


def apply_texture_set(
    loader,
    node: NodePath,
    name: str,
    textures: TextureSet,
    *,
    uv_scale: tuple[float, float] | None = None,
    roughness: float = 1.0,
    metallic: float = 1.0,
    base_color=(1.0, 1.0, 1.0, 1.0),
    texcoord_name: str | None = None,
) -> Material:
    """Bind a base/normal/ORM texture set and its material to one node hierarchy.

    ``roughness`` and ``metallic`` scale the ORM map rather than replacing it. They default
    to 1.0 so the committed map drives the surface response; lower them only to deliberately
    bias a whole material.
    """

    base_stage = _stage(f"{name}-base-color", TextureStage.MModulate, texcoord_name)
    node.setTexture(base_stage, load_texture(loader, textures.base_color, repeat=True, srgb=True), 1)
    stages = [base_stage]

    if textures.normal is not None:
        normal_stage = _stage(f"{name}-normal", TextureStage.MNormal, texcoord_name)
        node.setTexture(normal_stage, load_texture(loader, textures.normal, repeat=True), 1)
        stages.append(normal_stage)

    if textures.orm is not None:
        orm_stage = _stage(f"{name}-orm", TextureStage.MSelector, texcoord_name)
        node.setTexture(orm_stage, load_texture(loader, textures.orm, repeat=True), 1)
        stages.append(orm_stage)

    if uv_scale is not None:
        for stage in stages:
            node.setTexScale(stage, *uv_scale)

    material = Material(f"{name}-material")
    material.setBaseColor(base_color)
    material.setRoughness(roughness)
    material.setMetallic(metallic)
    node.setMaterial(material, 1)
    return material


def apply_flat_material(
    node: NodePath,
    name: str,
    color,
    *,
    roughness: float = 0.85,
    metallic: float = 0.0,
    emission=None,
) -> Material:
    """Apply an untextured material for geometry with no committed texture set."""

    material = Material(name)
    material.setBaseColor(color)
    material.setRoughness(roughness)
    material.setMetallic(metallic)
    if emission is not None:
        material.setEmission(emission)
    node.setMaterial(material, 1)
    node.setColor(*color)
    return material
