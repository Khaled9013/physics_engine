"""Asset-backed outdoor range construction."""

from __future__ import annotations

from panda3d.core import NodePath

from .assets import VisualAssetPaths, load_texture
from .procedural import add_box, add_sphere, add_textured_card


class RangeEnvironment:
    """Build and own deterministic decorative range geometry."""

    def __init__(self, base, parent: NodePath, assets: VisualAssetPaths) -> None:
        self.base = base
        self.parent = parent
        self.assets = assets
        self.root = parent.attachNewNode("range-environment")
        self._build_ground()
        self._build_lane()
        self._build_firing_line()
        self._build_berms_and_horizon()
        self._build_range_dressing()

    def _build_ground(self) -> None:
        grass_texture = load_texture(self.base.loader, self.assets.grass_diffuse, repeat=True)
        ground = add_textured_card(
            self.root,
            "sparse-grass-ground",
            (-90.0, 90.0, -35.0, 1120.0),
            grass_texture,
            (42.0, 260.0),
        )
        ground.setP(-90.0)
        ground.setColorScale(0.58, 0.68, 0.54, 1.0)
        ground.setZ(-0.035)

    def _build_lane(self) -> None:
        loader = self.base.loader
        gravel_texture = load_texture(self.base.loader, self.assets.gravel_diffuse, repeat=True)
        lane = add_textured_card(
            self.root,
            "gravel-range-lane",
            (-7.0, 7.0, -8.0, 1050.0),
            gravel_texture,
            (5.0, 250.0),
        )
        lane.setP(-90.0)
        lane.setColorScale(0.48, 0.43, 0.34, 1.0)
        lane.setZ(0.005)
        for lateral in (-7.15, 7.15):
            add_box(
                loader,
                self.root,
                "lane-curb",
                (lateral, 520.0, 0.08),
                (0.13, 528.0, 0.08),
                (0.63, 0.61, 0.55, 1.0),
            )
        for distance in range(50, 1001, 50):
            width = 0.16 if distance % 100 else 0.28
            color = (0.78, 0.74, 0.62, 1.0) if distance % 100 else (0.92, 0.78, 0.30, 1.0)
            add_box(
                loader,
                self.root,
                "distance-stripe",
                (0.0, float(distance), 0.055),
                (7.0, width, 0.025),
                color,
            )

    def _build_firing_line(self) -> None:
        loader = self.base.loader
        add_box(
            loader,
            self.root,
            "firing-pad",
            (0.0, -1.5, 0.06),
            (5.7, 4.8, 0.06),
            (0.24, 0.25, 0.24, 1.0),
        )
        for lateral in (-5.7, 5.7):
            add_box(
                loader,
                self.root,
                "firing-bay-post",
                (lateral, 1.3, 1.7),
                (0.12, 0.12, 1.7),
                (0.12, 0.14, 0.14, 1.0),
            )
        add_box(
            loader,
            self.root,
            "firing-bay-rail",
            (0.0, 2.6, 0.38),
            (5.8, 0.08, 0.06),
            (0.20, 0.22, 0.22, 1.0),
        )
        for lateral in (-5.0, 5.0):
            add_box(
                loader,
                self.root,
                "equipment-case",
                (lateral, 3.3, 0.35),
                (0.65, 0.9, 0.35),
                (0.18, 0.23, 0.18, 1.0),
            )

    def _build_berms_and_horizon(self) -> None:
        loader = self.base.loader
        for lateral in (-31.0, 31.0):
            add_sphere(
                loader,
                self.root,
                "side-berm",
                (lateral, 530.0, 1.2),
                (25.0, 545.0, 4.8),
                (0.20, 0.28, 0.16, 1.0),
            )
        add_sphere(
            loader,
            self.root,
            "backstop-earth",
            (0.0, 1035.0, 4.0),
            (72.0, 28.0, 12.0),
            (0.25, 0.29, 0.19, 1.0),
        )
        mountain_data = (
            (-150.0, 1320.0, 28.0, 130.0, 95.0, 52.0),
            (-20.0, 1450.0, 34.0, 170.0, 120.0, 62.0),
            (145.0, 1360.0, 24.0, 145.0, 105.0, 48.0),
        )
        for x, y, z, sx, sy, sz in mountain_data:
            add_sphere(
                loader,
                self.root,
                "distant-ridge",
                (x, y, z),
                (sx, sy, sz),
                (0.25, 0.31, 0.29, 1.0),
            )

    def _build_range_dressing(self) -> None:
        tree_positions = (
            (-18.0, 42.0, 1.0),
            (21.0, 65.0, 1.2),
            (-27.0, 105.0, 1.3),
            (30.0, 145.0, 1.1),
            (-37.0, 225.0, 1.4),
            (39.0, 310.0, 1.3),
            (-45.0, 410.0, 1.5),
            (47.0, 530.0, 1.4),
        )
        for x, y, scale in tree_positions:
            self._add_tree(x, y, scale)
        for lateral, distance in ((-4.6, 100.0), (4.8, 250.0), (-4.7, 500.0)):
            self._add_background_target(lateral, distance)

    def _add_tree(self, lateral: float, distance: float, scale: float) -> None:
        loader = self.base.loader
        root = self.root.attachNewNode("range-tree")
        root.setPos(lateral, distance, 0.0)
        add_box(
            loader,
            root,
            "tree-trunk",
            (0.0, 0.0, 1.5 * scale),
            (0.22 * scale, 0.22 * scale, 1.5 * scale),
            (0.24, 0.15, 0.08, 1.0),
        )
        for offset_x, offset_y, offset_z, radius in (
            (0.0, 0.0, 4.1, 2.0),
            (-1.0, 0.15, 3.8, 1.5),
            (1.0, -0.1, 3.7, 1.45),
        ):
            add_sphere(
                loader,
                root,
                "tree-canopy",
                (offset_x * scale, offset_y * scale, offset_z * scale),
                (radius * scale, radius * scale, radius * 0.9 * scale),
                (0.16, 0.28, 0.14, 1.0),
            )

    def _add_background_target(self, lateral: float, distance: float) -> None:
        loader = self.base.loader
        root = self.root.attachNewNode("background-target-bay")
        root.setPos(lateral, distance, 0.0)
        add_box(
            loader,
            root,
            "target-post",
            (0.0, 0.0, 1.15),
            (0.06, 0.06, 1.15),
            (0.29, 0.23, 0.15, 1.0),
        )
        add_box(
            loader,
            root,
            "target-board",
            (0.0, 0.0, 2.25),
            (0.75, 0.07, 0.75),
            (0.78, 0.77, 0.70, 1.0),
        )
        add_sphere(
            loader,
            root,
            "target-plate",
            (0.0, -0.085, 2.25),
            (0.38, 0.035, 0.38),
            (0.32, 0.37, 0.38, 1.0),
        )

    def destroy(self) -> None:
        self.root.removeNode()
