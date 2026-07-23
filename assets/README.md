# Third-Party Visual Assets

These files are presentation assets for the local Panda3D application. They are never read by
`ballistics_core` and are not used as physical dimensions or solver inputs.

## WRAD ARMS

- Publisher: wriks
- Source: https://wriks.itch.io/wrad-arms
- License: CC0 1.0 Universal; the publisher page and included `LICENSE.txt` both state CC0.
- Original archive: `WRAD_ARMS.zip`
- Archive SHA-256: `d5f91fdc9bd6465dc59b54265ca2aefe2dc881c00b6c7a9c76bd0b80fa498093`
- Vendored source: `third_party/wrad_arms/source/arms.glb`
- Runtime conversion: `gltf2bam` from panda3d-gltf 1.3.0.
- Local presentation change: the embedded pale albedo is replaced at runtime by the included dark
  albedo supplied in the same archive. Geometry, rig, and UVs are unchanged.

## Stein Games precision rifle

- Publisher: Stein Games
- Source: https://stein-indie.itch.io/m700
- License: CC0 1.0 Universal; see the publisher page and included `LICENSE.txt`.
- Original archive: `Sniper_R700_Free_CC0.zip`
- Archive SHA-256: `67e960d12c1fe7570469ccb295b92f27f11fa26aa9a63bda4fe1de807d22f319`
- Vendored source: `third_party/stein_sniper_r700/source/SKM_SniperR700.fbx`
- Conversion: FBX2glTF 0.9.7 creates binary glTF; the unused skeletal skin declaration is removed
  with `scripts/strip_gltf_skin.py`; panda3d-gltf 1.3.0 then creates the runtime BAM.
- The FBX2glTF project is BSD-3-Clause and is used only as an offline conversion tool.
- Local texture conversion: the original MAOR mask stores metallic in red, ambient occlusion in
  green, and inverted roughness in blue. `rifle_orm.png` repacks those channels to occlusion in
  red, roughness in green, and metallic in blue for the runtime PBR shader.
- The artwork depicts a generic fictionalized precision rifle in this application. No real model
  name, operating information, or profile is exposed in the user interface.

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
initialization with a clear message instead of silently restoring the old block-built view model.
