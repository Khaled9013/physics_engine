"""Asset-backed outdoor range construction.

This module is a builder. Every position, size, and colour it uses comes from
`range_layout`, every surface response comes from `materials`, and every ground mesh comes
from `terrain`. Keeping content out of the builder is what allows the layout to be inspected
without a graphics context.
"""

from __future__ import annotations

from panda3d.core import NodePath, TransparencyAttrib

from . import range_layout as layout
from .assets import VisualAssetPaths
from .materials import apply_flat_material, apply_texture_set
from .procedural import (
    add_box,
    add_cross_billboard,
    add_sphere,
    add_textured_card,
    make_canopy_texture,
    make_contact_shadow_texture,
    make_grass_tuft_texture,
    make_target_face_texture,
)
from .terrain import build_terrain, lane_shading


# Two canopy cut-outs, alternated per tree, so a row of trees is not one silhouette repeated.
CANOPY_VARIANT_SEEDS = (0x5EED_0002, 0x5EED_0012)


class RangeEnvironment:
    """Build and own deterministic decorative range geometry."""

    def __init__(self, base, parent: NodePath, assets: VisualAssetPaths) -> None:
        self.base = base
        self.parent = parent
        self.assets = assets
        self.root = parent.attachNewNode("range-environment")
        self.target_face_texture = make_target_face_texture()
        self._canopy_textures = tuple(
            make_canopy_texture(seed=seed) for seed in CANOPY_VARIANT_SEEDS
        )
        self._tuft_texture = make_grass_tuft_texture()
        self._contact_shadow_texture = make_contact_shadow_texture()
        self._build_ground()
        self._build_lane()
        self._build_firing_line()
        self._build_landforms()
        self._build_vegetation()
        self._build_background_bays()

    def _build_ground(self) -> None:
        loader = self.base.loader
        for band, textures in (
            (layout.NEAR_FIELD, self.assets.grass),
            (layout.FAR_FIELD, self.assets.macro_ground),
        ):
            mesh = build_terrain(
                self.root,
                band.name,
                x_range=band.x_range,
                y_range=band.y_range,
                divisions=band.divisions,
                tile_size_m=band.tile_size_m,
                amplitude=band.amplitude,
                y_bias=band.y_bias,
            )
            apply_texture_set(loader, mesh, band.name, textures, base_color=band.tint)

    def _build_lane(self) -> None:
        loader = self.base.loader
        lane = build_terrain(
            self.root,
            "gravel-range-lane",
            x_range=(-layout.LANE_HALF_WIDTH_M, layout.LANE_HALF_WIDTH_M),
            y_range=(layout.LANE_START_M, layout.LANE_END_M),
            divisions=(10, 96),
            tile_size_m=layout.LANE_TILE_SIZE_M,
            amplitude=0.0,
            y_bias=1.35,
            shading=lane_shading,
        )
        apply_texture_set(
            loader, lane, "gravel-range-lane", self.assets.gravel,
            base_color=layout.LANE_TINT,
        )
        lane.setZ(layout.LANE_SURFACE_Z)
        for marker in layout.distance_markers():
            add_box(loader, self.root, marker.name, marker.position, marker.scale, marker.color)

    def _build_firing_line(self) -> None:
        loader = self.base.loader
        for placement in layout.FIRING_LINE:
            add_box(
                loader,
                self.root,
                placement.name,
                placement.position,
                placement.scale,
                placement.color,
            )

    def _build_landforms(self) -> None:
        loader = self.base.loader
        for berm in layout.BERMS:
            add_sphere(loader, self.root, "side-berm", berm.position, berm.scale, berm.color)
        ridge_root = self.root.attachNewNode("distant-ridges")
        for ridge in layout.RIDGES:
            add_sphere(
                loader, ridge_root, "distant-ridge", ridge.position, ridge.scale, ridge.color
            )

    def _add_contact_shadow(self, parent: NodePath, radius: float) -> NodePath:
        """Lay a soft dark patch on the ground so an object does not appear to hover."""

        patch = add_textured_card(
            parent, "contact-shadow", (-radius, radius, -radius, radius), self._contact_shadow_texture
        )
        patch.setP(-90.0)
        patch.setZ(0.02)
        patch.setTransparency(TransparencyAttrib.MAlpha)
        patch.setLightOff(1)
        patch.setDepthWrite(False)
        patch.setBin("transparent", 10)
        return patch

    def _build_vegetation(self) -> None:
        tree_root = self.root.attachNewNode("range-vegetation")
        for index, tree in enumerate(layout.TREES):
            self._add_tree(tree_root, tree, index)
        tuft_root = self.root.attachNewNode("ground-clutter")
        # One prototype instanced per placement. Building a fresh pair of cards for every
        # tuft dominated scene construction, and the geometry is identical anyway; scale and
        # heading vary per instance, which is what breaks up the repetition.
        prototype = add_cross_billboard(
            self.root.attachNewNode("grass-tuft-prototype"),
            "grass-tuft",
            self._tuft_texture,
            0.62,
            0.46,
        )
        prototype.getParent().hide()
        for x, y, scale, heading in layout.grass_tufts():
            placement = tuft_root.attachNewNode("grass-tuft-instance")
            placement.setPos(x, y, 0.0)
            placement.setH(heading)
            placement.setScale(scale)
            prototype.instanceTo(placement)

    def _add_tree(self, parent: NodePath, tree: layout.TreePlacement, index: int) -> None:
        root = parent.attachNewNode("range-tree")
        root.setPos(tree.lateral, tree.distance, 0.0)
        # The lean is applied to the trunk rather than the whole tree, so the contact shadow
        # stays flat on the ground instead of tilting with it.
        trunk_height = 2.6 * tree.scale
        trunk = add_box(
            self.base.loader,
            root,
            "tree-trunk",
            (0.0, 0.0, trunk_height * 0.5),
            (0.16 * tree.scale, 0.16 * tree.scale, trunk_height * 0.5),
            (0.19, 0.15, 0.11, 1.0),
        )
        trunk.setR(tree.lean_deg)
        canopy = add_cross_billboard(
            root,
            "tree-canopy",
            self._canopy_textures[index % len(self._canopy_textures)],
            5.4 * tree.scale,
            5.0 * tree.scale,
        )
        canopy.setZ(trunk_height * 0.72)
        canopy.setR(tree.lean_deg)
        # A small deterministic tint spread stops a row of trees reading as one repeated
        # object, which is the clearest tell that vegetation is instanced.
        tint = 0.86 + ((index * 0.37) % 1.0) * 0.30
        canopy.setColorScale(tint, tint * (0.94 + (index % 3) * 0.04), tint * 0.92, 1.0)
        self._add_contact_shadow(root, 2.4 * tree.scale)

    def _build_background_bays(self) -> None:
        loader = self.base.loader
        for bay in layout.TARGET_BAYS:
            root = self.root.attachNewNode("background-target-bay")
            root.setPos(bay.lateral, bay.distance, 0.0)
            for lateral in (-0.62, 0.62):
                add_box(
                    loader,
                    root,
                    "target-post",
                    (lateral, 0.0, 0.62),
                    (0.045, 0.045, 0.62),
                    (0.26, 0.21, 0.14, 1.0),
                )
            board = add_box(
                loader,
                root,
                "target-board",
                (0.0, 0.0, 1.42),
                (0.70, 0.035, 0.70),
                (0.62, 0.60, 0.55, 1.0),
            )
            board.setTwoSided(True)
            face = add_textured_card(
                root,
                "background-target-face",
                (-0.62, 0.62, -0.62, 0.62),
                self.target_face_texture,
            )
            face.setPos(0.0, -0.042, 1.42)
            face.setTransparency(TransparencyAttrib.MAlpha)
            apply_flat_material(face, "background-target-face-material", (1.0, 1.0, 1.0, 1.0))
            face.clearColor()

    def destroy(self) -> None:
        self.root.removeNode()
