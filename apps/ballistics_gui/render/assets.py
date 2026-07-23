"""Local visual-asset resolution and validated Panda3D loading."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from panda3d.core import Filename, SamplerState, Texture


REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
ASSET_ROOT = REPOSITORY_ROOT / "assets" / "third_party"


@dataclass(frozen=True)
class VisualAssetPaths:
    hands_model: Path
    rifle_model: Path
    rifle_base_color: Path
    rifle_normal: Path
    rifle_orm: Path
    scope_model: Path
    grass_diffuse: Path
    gravel_diffuse: Path


def visual_asset_paths() -> VisualAssetPaths:
    """Return absolute paths to the committed, offline visual assets."""

    return VisualAssetPaths(
        hands_model=ASSET_ROOT
        / "open_game_art_fps_hands"
        / "runtime"
        / "fps_rifle_hands.bam",
        rifle_model=ASSET_ROOT
        / "stein_sniper_r700"
        / "runtime"
        / "sniper_r700.bam",
        rifle_base_color=ASSET_ROOT
        / "stein_sniper_r700"
        / "textures"
        / "rifle_base_color.png",
        rifle_normal=ASSET_ROOT
        / "stein_sniper_r700"
        / "textures"
        / "rifle_normal.png",
        rifle_orm=ASSET_ROOT
        / "stein_sniper_r700"
        / "textures"
        / "rifle_orm.png",
        scope_model=ASSET_ROOT
        / "poly_pizza_scope"
        / "runtime"
        / "rifle_scope.bam",
        grass_diffuse=ASSET_ROOT
        / "poly_haven_ground"
        / "textures"
        / "sparse_grass_diff_1k.jpg",
        gravel_diffuse=ASSET_ROOT
        / "poly_haven_ground"
        / "textures"
        / "gravel_floor_02_diff_1k.jpg",
    )


def require_asset(path: Path) -> Path:
    """Validate one local asset and return it unchanged."""

    if not path.is_file():
        raise RuntimeError(f"required visual asset is missing: {path}")
    return path


def load_model(loader, path: Path):
    """Load a required model and reject Panda3D's empty failure node."""

    checked_path = require_asset(path)
    model = loader.loadModel(Filename.fromOsSpecific(str(checked_path)))
    if model is None or model.isEmpty():
        raise RuntimeError(f"Panda3D could not load visual asset: {checked_path}")
    return model


def load_texture(loader, path: Path, *, repeat: bool = False) -> Texture:
    """Load a required texture with stable filtering settings."""

    checked_path = require_asset(path)
    texture = loader.loadTexture(Filename.fromOsSpecific(str(checked_path)))
    if texture is None:
        raise RuntimeError(f"Panda3D could not load texture: {checked_path}")
    texture.setMinfilter(SamplerState.FT_linear_mipmap_linear)
    texture.setMagfilter(SamplerState.FT_linear)
    texture.setAnisotropicDegree(8)
    if repeat:
        texture.setWrapU(SamplerState.WM_repeat)
        texture.setWrapV(SamplerState.WM_repeat)
    return texture
