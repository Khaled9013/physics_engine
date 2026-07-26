# Third-Party Visual Assets

These files are presentation assets for the local Panda3D application. They are never read by
`ballistics_core` and are not used as physical dimensions or solver inputs. Source names are
preserved here for provenance; the application presents only a fictional training profile.

## OpenGameArt first-person hands

- Author: Robin Lamb
- Source: https://opengameart.org/content/low-poly-fps-rifle-and-hands
- License: CC0 1.0 Universal, as stated on the source page.
- Original archive: `rifle_0.zip`
- Archive SHA-256: `fc6c9f95513dacbce7931bda65e373a29cedadcc9897cb33662851ab76c76a14`
- Vendored source: `third_party/open_game_art_fps_hands/source/fps_rifle_hands.glb`
- Source GLB SHA-256: `96fb56cca8bcc2c73e0c3870924d8b1bfd2986b5f4fc4aca3e8483d57e656b0c`
- Runtime conversion: `gltf2bam` from panda3d-gltf 1.3.0.
- Runtime treatment: the separable proxy rifle is hidden; its authored hand pose and short
  fire animation are combined with the detailed rifle below.

## Stein Games precision rifle

- Publisher: Stein Games
- Source: https://stein-indie.itch.io/m700
- License: CC0 1.0 Universal; see the publisher page and included `LICENSE.txt`.
- Original archive: `Sniper_R700_Free_CC0.zip`
- Archive SHA-256: `67e960d12c1fe7570469ccb295b92f27f11fa26aa9a63bda4fe1de807d22f319`
- Vendored source: `third_party/stein_sniper_r700/source/SKM_SniperR700.fbx`
- Conversion: FBX2glTF 0.9.7 creates binary glTF; the unused skeletal skin declaration is removed
  with `scripts/strip_gltf_skin.py`; panda3d-gltf 1.3.0 then creates the runtime BAM.
- FBX2glTF is BSD-3-Clause and is used only as an offline conversion tool.
- The original MAOR mask stores metallic in red, ambient occlusion in green, and inverted
  roughness in blue. `rifle_orm.png` repacks those channels to occlusion in red, roughness in
  green, and metallic in blue for PBR-compatible future use.
- The artwork depicts a generic fictionalized precision rifle in this application. No real model
  name, operating information, or profile is exposed in the interface.

## Poly Pizza optic

- Author: Pichuliru
- Source: https://poly.pizza/m/98ocxnqLFf
- License: CC0 1.0 Universal, as stated on the source page.
- Official download SHA-256: `5a345029d105de394f79a837a86589ff90d3a17acf86f67409f2d8a121771e2a`
- Vendored source: `third_party/poly_pizza_scope/source/rifle_scope.glb`
- Runtime conversion: `gltf2bam` from panda3d-gltf 1.3.0.

## Poly Haven ground materials

- Sparse Grass: https://polyhaven.com/a/sparse_grass
- Gravel Floor 02: https://polyhaven.com/a/gravel_floor_02
- Aerial Grass Rock: https://polyhaven.com/a/aerial_grass_rock
- Publisher license statement: https://docs.polyhaven.com/en/faq
- License: CC0.
- Files are official 1K JPG downloads, unmodified.
- The `arm` maps pack ambient occlusion, roughness, and metalness into the red, green, and
  blue channels. That is the layout `simplepbr` samples from `p3d_TextureMetalRoughness`, so
  they are bound directly with no repacking.
- The `nor_gl` maps are OpenGL-convention tangent-space normals, matching Panda3D.

| File | MD5 |
| --- | --- |
| `sparse_grass_diff_1k.jpg` | `5d0aba796e1b5882555161c16b10ee9d` |
| `sparse_grass_nor_gl_1k.jpg` | `3307e6ce47413c6b4e85885d4b8ae932` |
| `sparse_grass_arm_1k.jpg` | `e14c82e03d13fb472315d74a28196c89` |
| `gravel_floor_02_diff_1k.jpg` | `9e420814367b8ee7a25bcfea10ff4b08` |
| `gravel_floor_02_nor_gl_1k.jpg` | `ed3da750848a3e05faff98bc5b08c491` |
| `gravel_floor_02_arm_1k.jpg` | `985dce4fb099a723dc083286d2ed757a` |
| `aerial_grass_rock_diff_1k.jpg` | `e920ce36afd0abff000b8366d3d768d3` |
| `aerial_grass_rock_nor_gl_1k.jpg` | `c8aa4c4f09b113cc7edef89ddeaccad9` |
| `aerial_grass_rock_arm_1k.jpg` | `1f37fdb9b46b7fe34932ed9aa77df0bf` |

## Poly Haven sky

- Kloofendal Overcast Puresky: https://polyhaven.com/a/kloofendal_overcast_puresky
- Publisher license statement: https://docs.polyhaven.com/en/faq
- License: CC0.
- Source: the publisher's tonemapped equirectangular JPG, 8192x4096.
- Source MD5: `61b14cf4deed54ca8bd1a0c3314f8a21`
- The tonemapped JPG is used rather than the HDR original so no high-dynamic-range decoder
  is needed at conversion time and no extra dependency enters the toolchain.
- The 16.9 MB source panorama is not committed; it is far larger than everything derived
  from it, and the URL and checksum above reproduce it exactly.
- Runtime conversion: `scripts/prepare_sky_texture.py`, which uses Panda3D's own `PNMImage`
  and therefore needs nothing beyond the existing GUI requirements. It writes:
  - `runtime/overcast_sky_2k.jpg`, a 2048x1024 panorama for the visible sky dome.
  - `runtime/overcast_sky_cube_{0..5}.jpg`, 256x256 cube faces projected from the panorama
    and consumed by `simplepbr` for image-based lighting.
- Regenerate both products with:

  ```
  .venv/bin/python scripts/prepare_sky_texture.py \
      <source.jpg> assets/third_party/poly_haven_sky/runtime/overcast_sky_2k.jpg \
      --cube-destination assets/third_party/poly_haven_sky/runtime/overcast_sky_cube_#.jpg
  ```

## Runtime policy

Asset acquisition and conversion are development-time tasks. The GUI resolves committed files from
the local repository/package and performs no network access. Missing hero assets fail renderer
initialization with a clear message instead of silently restoring placeholder geometry.
