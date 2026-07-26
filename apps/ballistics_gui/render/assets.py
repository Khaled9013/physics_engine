"""Local visual-asset resolution and validated Panda3D loading."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from panda3d.core import Filename, SamplerState, Texture


REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
ASSET_ROOT = REPOSITORY_ROOT / "assets" / "third_party"

CUBE_FACE_COUNT = 6


@dataclass(frozen=True)
class TextureSet:
    """One physically-based material's committed image maps.

    ``orm`` packs ambient occlusion, roughness, and metalness in the red, green, and blue
    channels respectively, matching both Poly Haven's ``arm`` export and the channel layout
    `simplepbr` samples from ``p3d_TextureMetalRoughness``.
    """

    base_color: Path
    normal: Path | None = None
    orm: Path | None = None


@dataclass(frozen=True)
class SkyTextures:
    """The visible sky panorama and its image-based-lighting cube faces."""

    panorama: Path
    cube_faces: tuple[Path, ...]

    def cube_pattern(self) -> Path:
        """Return the ``#``-templated path Panda3D's cube-map loader expects."""

        first = self.cube_faces[0]
        return first.with_name(first.name.replace("_0.", "_#.", 1))


@dataclass(frozen=True)
class VisualAssetPaths:
    hands_model: Path
    rifle_model: Path
    rifle: TextureSet
    scope_model: Path
    grass: TextureSet
    gravel: TextureSet
    macro_ground: TextureSet
    sky: SkyTextures


def visual_asset_paths() -> VisualAssetPaths:
    """Return absolute paths to the committed, offline visual assets."""

    ground = ASSET_ROOT / "poly_haven_ground" / "textures"
    sky = ASSET_ROOT / "poly_haven_sky" / "runtime"
    rifle = ASSET_ROOT / "stein_sniper_r700"
    return VisualAssetPaths(
        hands_model=ASSET_ROOT
        / "open_game_art_fps_hands"
        / "runtime"
        / "fps_rifle_hands.bam",
        rifle_model=rifle / "runtime" / "sniper_r700.bam",
        rifle=TextureSet(
            base_color=rifle / "textures" / "rifle_base_color.png",
            normal=rifle / "textures" / "rifle_normal.png",
            orm=rifle / "textures" / "rifle_orm.png",
        ),
        scope_model=ASSET_ROOT / "poly_pizza_scope" / "runtime" / "rifle_scope.bam",
        grass=TextureSet(
            base_color=ground / "sparse_grass_diff_1k.jpg",
            normal=ground / "sparse_grass_nor_gl_1k.jpg",
            orm=ground / "sparse_grass_arm_1k.jpg",
        ),
        gravel=TextureSet(
            base_color=ground / "gravel_floor_02_diff_1k.jpg",
            normal=ground / "gravel_floor_02_nor_gl_1k.jpg",
            orm=ground / "gravel_floor_02_arm_1k.jpg",
        ),
        macro_ground=TextureSet(
            base_color=ground / "aerial_grass_rock_diff_1k.jpg",
            normal=ground / "aerial_grass_rock_nor_gl_1k.jpg",
            orm=ground / "aerial_grass_rock_arm_1k.jpg",
        ),
        sky=SkyTextures(
            panorama=sky / "overcast_sky_2k.jpg",
            cube_faces=tuple(
                sky / f"overcast_sky_cube_{index}.jpg" for index in range(CUBE_FACE_COUNT)
            ),
        ),
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


def load_texture(
    loader,
    path: Path,
    *,
    repeat: bool = False,
    srgb: bool = False,
) -> Texture:
    """Load a required texture with stable filtering settings.

    ``srgb`` marks colour maps so the renderer linearises them before lighting. Normal and
    ORM maps carry measurements rather than colour and must stay in their stored encoding.
    """

    checked_path = require_asset(path)
    texture = loader.loadTexture(Filename.fromOsSpecific(str(checked_path)))
    if texture is None:
        raise RuntimeError(f"Panda3D could not load texture: {checked_path}")
    texture.setMinfilter(SamplerState.FT_linear_mipmap_linear)
    texture.setMagfilter(SamplerState.FT_linear)
    texture.setAnisotropicDegree(8)
    if srgb:
        texture.setFormat(Texture.F_srgb_alpha if texture.getNumComponents() == 4 else Texture.F_srgb)
    if repeat:
        texture.setWrapU(SamplerState.WM_repeat)
        texture.setWrapV(SamplerState.WM_repeat)
    return texture
