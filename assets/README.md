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
- Publisher license statement: https://docs.polyhaven.com/en/faq
- License: CC0.
- Files are official 1K diffuse JPG downloads.
- Sparse Grass MD5: `5d0aba796e1b5882555161c16b10ee9d`
- Gravel Floor 02 MD5: `9e420814367b8ee7a25bcfea10ff4b08`

## Runtime policy

Asset acquisition and conversion are development-time tasks. The GUI resolves committed files from
the local repository/package and performs no network access. Missing hero assets fail renderer
initialization with a clear message instead of silently restoring placeholder geometry.
