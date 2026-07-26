"""Sky dome and distance haze.

The dome is a camera-following inverted sphere carrying the committed equirectangular
panorama. It is unlit and depth-inert so it always reads as infinitely distant regardless of
where the shot or the terrain sits.

Haze uses `simplepbr`'s in-shader exponential fog. Panda3D's `render.setFog` alone has no
effect under the physically-based shader; the pipeline must also be built with fog enabled,
which is why `RangeLighting` and this module are constructed together by the scene.
"""

from __future__ import annotations

import math

from panda3d.core import (
    Fog,
    Geom,
    GeomNode,
    GeomTriangles,
    GeomVertexData,
    GeomVertexFormat,
    GeomVertexWriter,
    NodePath,
    SamplerState,
)

from .assets import SkyTextures, load_texture


# Overcast haze is dense enough to separate the near lane from the far backstop without
# hiding the target at the longest supported range.
HAZE_DENSITY = 0.00020
HAZE_COLOR = (0.43, 0.47, 0.53)

DOME_RADIUS = 1.0
DOME_MERIDIANS = 48
DOME_PARALLELS = 24


def _build_dome_geometry(name: str) -> GeomNode:
    """Generate an inward-facing UV sphere with equirectangular texture coordinates."""

    vertex_format = GeomVertexFormat.getV3n3t2()
    vertex_data = GeomVertexData(name, vertex_format, Geom.UHStatic)
    vertex_data.setNumRows((DOME_MERIDIANS + 1) * (DOME_PARALLELS + 1))
    vertex = GeomVertexWriter(vertex_data, "vertex")
    normal = GeomVertexWriter(vertex_data, "normal")
    texcoord = GeomVertexWriter(vertex_data, "texcoord")

    for parallel in range(DOME_PARALLELS + 1):
        v = parallel / DOME_PARALLELS
        polar = v * math.pi
        z = math.cos(polar)
        ring_radius = math.sin(polar)
        for meridian in range(DOME_MERIDIANS + 1):
            u = meridian / DOME_MERIDIANS
            azimuth = (u - 0.5) * 2.0 * math.pi
            x = ring_radius * math.sin(azimuth)
            y = ring_radius * math.cos(azimuth)
            vertex.addData3(x * DOME_RADIUS, y * DOME_RADIUS, z * DOME_RADIUS)
            normal.addData3(-x, -y, -z)
            texcoord.addData2(u, 1.0 - v)

    triangles = GeomTriangles(Geom.UHStatic)
    stride = DOME_MERIDIANS + 1
    for parallel in range(DOME_PARALLELS):
        for meridian in range(DOME_MERIDIANS):
            lower_left = parallel * stride + meridian
            lower_right = lower_left + 1
            upper_left = lower_left + stride
            upper_right = upper_left + 1
            triangles.addVertices(lower_left, upper_left, lower_right)
            triangles.addVertices(lower_right, upper_left, upper_right)

    geom = Geom(vertex_data)
    geom.addPrimitive(triangles)
    node = GeomNode(name)
    node.addGeom(geom)
    return node


class RangeSky:
    """Own the visible sky dome and the atmospheric haze applied to the world."""

    def __init__(self, base, textures: SkyTextures) -> None:
        self.base = base
        self.root = base.camera.attachNewNode(_build_dome_geometry("range-sky-dome"))
        # Sit just inside the far plane and follow the camera's position, so the dome can
        # never be entered, clipped, or overtaken by distant terrain.
        self.root.setScale(base.camLens.getFar() * 0.92)
        self.root.setCompass()
        self.root.setLightOff(1)
        self.root.setShaderOff(1)
        self.root.setFogOff(1)
        self.root.setDepthWrite(False)
        self.root.setDepthTest(False)
        self.root.setBin("background", 0)
        # Rendered two-sided rather than relying on a cull mode matching the generated
        # winding: the dome is a single low-density mesh, so the saved fill is irrelevant
        # next to the risk of silently culling the entire sky.
        self.root.setTwoSided(True)

        panorama = load_texture(base.loader, textures.panorama, srgb=True)
        panorama.setWrapU(SamplerState.WM_repeat)
        panorama.setWrapV(SamplerState.WM_clamp)
        self.root.setTexture(panorama, 1)

        self.fog = Fog("range-atmospheric-haze")
        self.fog.setColor(*HAZE_COLOR)
        self.fog.setExpDensity(HAZE_DENSITY)
        base.render.setFog(self.fog)

    def set_haze_density(self, density: float) -> None:
        self.fog.setExpDensity(density)

    def destroy(self) -> None:
        self.base.render.clearFog()
        self.root.removeNode()


def sky_root(parent: NodePath) -> NodePath:
    """Return a child node reserved for sky-attached decoration."""

    return parent.attachNewNode("sky-decoration")
