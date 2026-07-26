from __future__ import annotations

import dataclasses
from pathlib import Path
import unittest

from apps.ballistics_gui.render.assets import visual_asset_paths


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]


def _collect_paths(value, prefix: str = "") -> list[tuple[str, Path]]:
    """Walk the asset record and return every declared file path.

    Assets are grouped into nested records (texture sets, sky faces), so this recurses
    rather than reading one flat level. Any asset added later is covered automatically.
    """

    if isinstance(value, Path):
        return [(prefix, value)]
    if dataclasses.is_dataclass(value):
        found = []
        for field in dataclasses.fields(value):
            child = getattr(value, field.name)
            found.extend(_collect_paths(child, f"{prefix}.{field.name}" if prefix else field.name))
        return found
    if isinstance(value, (tuple, list)):
        found = []
        for index, child in enumerate(value):
            found.extend(_collect_paths(child, f"{prefix}[{index}]"))
        return found
    return []


class VisualAssetTests(unittest.TestCase):
    def test_every_runtime_asset_is_committed_and_nonempty(self) -> None:
        collected = _collect_paths(visual_asset_paths())
        self.assertGreaterEqual(len(collected), 15, "asset record collapsed to too few files")
        for name, path in collected:
            with self.subTest(asset=name):
                self.assertTrue(path.is_file(), path)
                self.assertGreater(path.stat().st_size, 1024, path)

    def test_sky_cube_pattern_matches_committed_faces(self) -> None:
        sky = visual_asset_paths().sky
        self.assertEqual(len(sky.cube_faces), 6)
        pattern = sky.cube_pattern()
        self.assertIn("#", pattern.name)
        for index, face in enumerate(sky.cube_faces):
            resolved = pattern.with_name(pattern.name.replace("#", str(index)))
            self.assertEqual(resolved, face)
            self.assertTrue(face.is_file(), face)

    def test_runtime_bam_models_have_valid_headers(self) -> None:
        paths = visual_asset_paths()
        for path in (paths.hands_model, paths.rifle_model, paths.scope_model):
            with self.subTest(model=path.name):
                self.assertEqual(path.read_bytes()[:4], b"pbj\x00")

    def test_gui_runtime_contains_no_network_endpoints(self) -> None:
        gui_root = REPOSITORY_ROOT / "apps" / "ballistics_gui"
        for path in gui_root.rglob("*.py"):
            source = path.read_text(encoding="utf-8")
            with self.subTest(module=path.relative_to(REPOSITORY_ROOT)):
                self.assertNotIn("http://", source)
                self.assertNotIn("https://", source)

    def test_asset_provenance_names_all_external_sources(self) -> None:
        provenance = (REPOSITORY_ROOT / "assets" / "README.md").read_text(
            encoding="utf-8"
        )
        for source_name in (
            "OpenGameArt",
            "Stein Games",
            "Poly Pizza",
            "Poly Haven",
        ):
            with self.subTest(source=source_name):
                self.assertIn(source_name, provenance)


if __name__ == "__main__":
    unittest.main()
