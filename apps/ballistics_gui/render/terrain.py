"""Ground mesh generation.

The previous range was a single flat `CardMaker` quad, which cannot carry a normal map (no
tangents) and reads as painted cardboard because a perfectly planar surface produces no
shading variation at all. This module generates a real triangle mesh with tangents, gently
rolling relief, and world-space texture coordinates.

Vertex spacing is graded rather than uniform: samples cluster near the firing line where the
camera sits and spread out downrange, so silhouette detail lands where it is visible without
paying for a uniform grid across a kilometre.

`simplepbr` exposes only one texture coordinate set, so a single mesh cannot blend two
materials. Repetition is instead broken by building the near field and far field as separate
meshes with different textures and tiling; the seam falls inside the haze.
"""

from __future__ import annotations

import math

from panda3d.core import (
    Geom,
    GeomNode,
    GeomTriangles,
    GeomVertexArrayFormat,
    GeomVertexData,
    GeomVertexFormat,
    GeomVertexWriter,
    InternalName,
    NodePath,
)


# The firing lane and its immediate surroundings stay flat so the shot, the target, and the
# distance markers are never occluded by decorative relief.
FLAT_LANE_HALF_WIDTH_M = 11.0
FLAT_LANE_FALLOFF_M = 26.0
FLAT_FIRING_LINE_M = 14.0


def _terrain_format() -> GeomVertexFormat:
    """Build a vertex format carrying tangents, which normal mapping requires."""

    array = GeomVertexArrayFormat()
    array.addColumn(InternalName.getVertex(), 3, Geom.NTFloat32, Geom.CPoint)
    array.addColumn(InternalName.getNormal(), 3, Geom.NTFloat32, Geom.CNormal)
    array.addColumn(InternalName.getTangent(), 3, Geom.NTFloat32, Geom.CVector)
    array.addColumn(InternalName.getBinormal(), 3, Geom.NTFloat32, Geom.CVector)
    array.addColumn(InternalName.getColor(), 4, Geom.NTFloat32, Geom.CColor)
    array.addColumn(InternalName.getTexcoord(), 2, Geom.NTFloat32, Geom.CTexcoord)
    return GeomVertexFormat.registerFormat(GeomVertexFormat(array))


def macro_shading(x: float, y: float) -> tuple[float, float, float]:
    """Return large-scale ground colour variation.

    A 1 k texture tiled across a field repeats visibly no matter how it is filtered, and
    `simplepbr` offers only one texture coordinate set, so a second blended material is not
    available. `simplepbr` does multiply vertex colour into base colour, so low-frequency
    variation written per vertex breaks the stamp for free: patches drift between drier and
    greener without any extra draw call or texture lookup.
    """

    broad = (
        math.sin(x * 0.0087 + 0.6) * math.cos(y * 0.0054 - 1.2) * 0.5
        + math.sin((x * 0.6 + y) * 0.0031 + 2.4) * 0.5
    )
    fine = math.sin(x * 0.041 - 0.8) * math.cos(y * 0.037 + 1.9) * 0.5
    value = 1.0 + broad * 0.34 + fine * 0.09
    # Drier patches lose more green than red, which is how sun-bleached grass actually shifts.
    green_bias = 1.0 + broad * 0.11
    return (value, value * green_bias, value * (1.0 - broad * 0.08))


def lane_shading(x: float, y: float) -> tuple[float, float, float]:
    """Return gravel wear: darker compacted edges and a lightly polished centre."""

    from_centre = abs(x) / 6.4
    edge = 1.0 - _smoothstep(0.55, 1.0, from_centre) * 0.28
    traffic = 1.0 + math.exp(-((x / 1.9) ** 2)) * 0.06
    settle = 1.0 + math.sin(y * 0.021 + 0.4) * 0.035
    value = edge * traffic * settle
    return (value, value * 0.995, value * 0.985)


def _smoothstep(edge0: float, edge1: float, value: float) -> float:
    if edge1 <= edge0:
        return 0.0 if value < edge0 else 1.0
    t = max(0.0, min(1.0, (value - edge0) / (edge1 - edge0)))
    return t * t * (3.0 - 2.0 * t)


def terrain_height(x: float, y: float, amplitude: float) -> float:
    """Return deterministic ground relief, flattened over the lane and firing line.

    The function is a fixed sum of sines rather than sampled noise so that every launch
    produces byte-identical geometry.
    """

    if amplitude <= 0.0:
        return 0.0
    lateral_fade = _smoothstep(
        FLAT_LANE_HALF_WIDTH_M, FLAT_LANE_HALF_WIDTH_M + FLAT_LANE_FALLOFF_M, abs(x)
    )
    downrange_fade = _smoothstep(-FLAT_FIRING_LINE_M, FLAT_FIRING_LINE_M * 2.0, y)
    relief = (
        math.sin(x * 0.0210 + 1.7) * math.cos(y * 0.0130 - 0.4) * 0.62
        + math.sin(x * 0.0061 - 0.9) * 1.05
        + math.cos(y * 0.0042 + 2.2) * 0.83
        + math.sin((x + y) * 0.0033 + 0.5) * 0.44
    )
    return relief * amplitude * lateral_fade * max(downrange_fade, 0.0)


def _graded_axis(minimum: float, maximum: float, divisions: int, bias: float) -> list[float]:
    """Distribute samples between two bounds, clustering them toward the minimum.

    ``bias`` of 1.0 is uniform; larger values push samples toward ``minimum``.
    """

    span = maximum - minimum
    return [minimum + span * ((step / divisions) ** bias) for step in range(divisions + 1)]


def _surface_normal_and_tangent(x: float, y: float, amplitude: float):
    """Derive the shading basis from finite differences of the height function."""

    delta = 0.75
    dz_dx = (
        terrain_height(x + delta, y, amplitude) - terrain_height(x - delta, y, amplitude)
    ) / (2.0 * delta)
    dz_dy = (
        terrain_height(x, y + delta, amplitude) - terrain_height(x, y - delta, amplitude)
    ) / (2.0 * delta)
    normal_length = math.sqrt(dz_dx * dz_dx + dz_dy * dz_dy + 1.0)
    normal = (-dz_dx / normal_length, -dz_dy / normal_length, 1.0 / normal_length)
    tangent_length = math.sqrt(1.0 + dz_dx * dz_dx)
    tangent = (1.0 / tangent_length, 0.0, dz_dx / tangent_length)
    binormal = (
        normal[1] * tangent[2] - normal[2] * tangent[1],
        normal[2] * tangent[0] - normal[0] * tangent[2],
        normal[0] * tangent[1] - normal[1] * tangent[0],
    )
    return normal, tangent, binormal


def build_terrain(
    parent: NodePath,
    name: str,
    *,
    x_range: tuple[float, float],
    y_range: tuple[float, float],
    divisions: tuple[int, int],
    tile_size_m: float,
    amplitude: float = 1.0,
    y_bias: float = 1.0,
    shading=macro_shading,
) -> NodePath:
    """Generate one ground mesh and attach it under ``parent``.

    ``tile_size_m`` is the world span of a single texture repeat, so tiling stays uniform
    even though vertex spacing is not.
    """

    x_divisions, y_divisions = divisions
    xs = [
        x_range[0] + (x_range[1] - x_range[0]) * step / x_divisions
        for step in range(x_divisions + 1)
    ]
    ys = _graded_axis(y_range[0], y_range[1], y_divisions, y_bias)

    vertex_data = GeomVertexData(name, _terrain_format(), Geom.UHStatic)
    vertex_data.setNumRows(len(xs) * len(ys))
    vertex = GeomVertexWriter(vertex_data, "vertex")
    normal = GeomVertexWriter(vertex_data, "normal")
    tangent = GeomVertexWriter(vertex_data, "tangent")
    binormal = GeomVertexWriter(vertex_data, "binormal")
    color = GeomVertexWriter(vertex_data, "color")
    texcoord = GeomVertexWriter(vertex_data, "texcoord")

    for y in ys:
        for x in xs:
            z = terrain_height(x, y, amplitude)
            surface_normal, surface_tangent, surface_binormal = _surface_normal_and_tangent(
                x, y, amplitude
            )
            vertex.addData3(x, y, z)
            normal.addData3(*surface_normal)
            tangent.addData3(*surface_tangent)
            binormal.addData3(*surface_binormal)
            color.addData4(*shading(x, y), 1.0)
            texcoord.addData2(x / tile_size_m, y / tile_size_m)

    triangles = GeomTriangles(Geom.UHStatic)
    stride = len(xs)
    for row in range(len(ys) - 1):
        for column in range(len(xs) - 1):
            lower_left = row * stride + column
            lower_right = lower_left + 1
            upper_left = lower_left + stride
            upper_right = upper_left + 1
            triangles.addVertices(lower_left, lower_right, upper_left)
            triangles.addVertices(lower_right, upper_right, upper_left)

    geom = Geom(vertex_data)
    geom.addPrimitive(triangles)
    node = GeomNode(name)
    node.addGeom(geom)
    return parent.attachNewNode(node)
