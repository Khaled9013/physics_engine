"""Outdoor lighting rig for the range.

The overcast direction is dominated by sky illumination rather than by the sun, so most of
the light comes from an image-based environment built from the committed sky cube faces. The
directional sun contributes soft shaping and the only shadow-casting frustum.

Panda3D's `DirectionalLight` ships an orthographic shadow lens whose default film covers a
one-unit square. On a kilometre-long range that leaves effectively nothing shadowed, so the
film is sized explicitly here and re-centred on the active target as the range changes.
"""

from __future__ import annotations

import math

from panda3d.core import AmbientLight, BitMask32, DirectionalLight, Vec3


# Geometry drawn by the main camera but excluded from this mask casts no shadow. The
# camera-space first-person rig uses it: the rig is presentation art at presentation scale,
# so its shadow would land on the ground at the wrong size and in the wrong place.
SHADOW_CASTER_MASK = BitMask32.bit(2)


SUN_HEADING_DEG = -34.0
SUN_PITCH_DEG = -21.0
SUN_COLOR_TEMPERATURE_K = 6200.0
SUN_INTENSITY = 1.45

# Overcast skies scatter a strong upward-facing bounce; a dim opposing light stands in for
# the ground-to-subject interreflection the environment map cannot express on its own.
BOUNCE_HEADING_DEG = 146.0
BOUNCE_PITCH_DEG = 34.0
BOUNCE_COLOR = (0.20, 0.21, 0.18, 1.0)

# Kept low because the environment map already supplies indirect diffuse; this only lifts
# surfaces the spherical-harmonic term leaves fully black.
AMBIENT_COLOR = (0.055, 0.062, 0.072, 1.0)

SHADOW_MAP_RESOLUTION = 2048
SHADOW_CASTER_DISTANCE_M = 220.0
SHADOW_FILM_NEAR_M = 90.0
SHADOW_FILM_FAR_M = 150.0
SHADOW_RANGE_FRACTION = 0.35
SHADOW_FOCUS_CAP_M = 42.0


class RangeLighting:
    """Own the sun, bounce fill, ambient floor, and shadow framing for the world."""

    def __init__(self, base) -> None:
        self.base = base
        render = base.render

        ambient = AmbientLight("range-ambient")
        ambient.setColor(AMBIENT_COLOR)
        self.ambient_node = render.attachNewNode(ambient)
        render.setLight(self.ambient_node)

        sun = DirectionalLight("range-sun")
        sun.setColorTemperature(SUN_COLOR_TEMPERATURE_K)
        sun.setColor(sun.getColor() * SUN_INTENSITY)
        sun.setShadowCaster(True, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION)
        sun.setCameraMask(SHADOW_CASTER_MASK)
        self.sun = sun
        self.sun_pivot = render.attachNewNode("range-sun-pivot")
        self.sun_node = self.sun_pivot.attachNewNode(sun)
        self.sun_node.setHpr(SUN_HEADING_DEG, SUN_PITCH_DEG, 0.0)
        self._place_sun()
        render.setLight(self.sun_node)

        bounce = DirectionalLight("range-bounce-fill")
        bounce.setColor(BOUNCE_COLOR)
        self.bounce_node = render.attachNewNode(bounce)
        self.bounce_node.setHpr(BOUNCE_HEADING_DEG, BOUNCE_PITCH_DEG, 0.0)
        render.setLight(self.bounce_node)

        self.set_focus_distance(150.0)

    def _place_sun(self) -> None:
        """Pull the shadow camera back along its own view direction."""

        forward = self.sun_node.getQuat().getForward()
        self.sun_node.setPos(-forward * SHADOW_CASTER_DISTANCE_M)

    def set_focus_distance(self, distance_m: float) -> None:
        """Centre the shadow volume between the firing line and the active target.

        The film widens with range so a distant target still falls inside the frustum, at
        the cost of shadow resolution that matters less as the subject shrinks on screen.
        """

        # Capped near the firing line: shadows read most strongly on the bench, posts, and
        # vegetation the shooter stands among. Centring on a fraction of a 1000 m target
        # would slide the whole frustum past everything the camera can actually resolve.
        focus_m = min(max(0.0, distance_m) * SHADOW_RANGE_FRACTION, SHADOW_FOCUS_CAP_M)
        self.sun_pivot.setPos(0.0, focus_m, 0.0)
        film = max(
            SHADOW_FILM_NEAR_M,
            min(SHADOW_FILM_FAR_M, 90.0 + distance_m * 0.20),
        )
        lens = self.sun.getLens()
        lens.setFilmSize(film, film)
        lens.setNearFar(1.0, SHADOW_CASTER_DISTANCE_M * 2.2)

    def sun_direction(self) -> Vec3:
        """Return the unit world-space direction the sun light travels along."""

        return self.sun_node.getQuat(self.base.render).getForward()

    def sun_elevation_deg(self) -> float:
        direction = self.sun_direction()
        return math.degrees(math.asin(max(-1.0, min(1.0, -direction.z))))

    def destroy(self) -> None:
        render = self.base.render
        for node in (self.ambient_node, self.sun_node, self.bounce_node):
            render.clearLight(node)
        self.sun_pivot.removeNode()
        self.ambient_node.removeNode()
        self.bounce_node.removeNode()
