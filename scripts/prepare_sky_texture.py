#!/usr/bin/env python3
"""Development-time conversion of a vendored tonemapped sky into runtime textures.

The Poly Haven tonemapped JPG is an equirectangular low-dynamic-range panorama at full
publisher resolution. This script produces two committed runtime products from it:

* a downscaled equirectangular JPG sampled by the visible sky dome, and
* six small cube-map faces consumed by ``simplepbr``'s image-based lighting.

The application never runs this script and never performs either conversion at launch.
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path
import sys

from panda3d.core import Filename, PNMImage


RUNTIME_WIDTH = 2048
RUNTIME_HEIGHT = 1024
CUBE_FACE_SIZE = 256

# Panda3D cube-map faces in load order, each mapping face-local (u, v) in [-1, 1] to a
# direction in Panda3D's Z-up world frame (+x right, +y forward, +z up).
CUBE_FACES = (
    ("positive_x", lambda u, v: (1.0, -u, -v)),
    ("negative_x", lambda u, v: (-1.0, u, -v)),
    ("positive_y", lambda u, v: (u, 1.0, -v)),
    ("negative_y", lambda u, v: (-u, -1.0, -v)),
    ("positive_z", lambda u, v: (u, -v, 1.0)),
    ("negative_z", lambda u, v: (u, v, -1.0)),
)


def _sample_equirectangular(panorama: PNMImage, x: float, y: float, z: float):
    """Sample an equirectangular panorama along one Z-up world direction."""

    length = math.sqrt(x * x + y * y + z * z)
    x, y, z = x / length, y / length, z / length
    longitude = math.atan2(x, y)
    latitude = math.asin(max(-1.0, min(1.0, z)))
    u = (longitude / (2.0 * math.pi)) + 0.5
    v = 0.5 - (latitude / math.pi)
    column = min(panorama.getXSize() - 1, max(0, int(u * panorama.getXSize())))
    row = min(panorama.getYSize() - 1, max(0, int(v * panorama.getYSize())))
    return panorama.getXel(column, row)


def _write_cube_faces(panorama: PNMImage, destination: Path, size: int) -> None:
    """Project the panorama onto six cube-map faces for image-based lighting."""

    destination.parent.mkdir(parents=True, exist_ok=True)
    for index, (name, direction) in enumerate(CUBE_FACES):
        face = PNMImage(size, size, 3)
        for row in range(size):
            v = (row + 0.5) / size * 2.0 - 1.0
            for column in range(size):
                u = (column + 0.5) / size * 2.0 - 1.0
                face.setXel(column, row, _sample_equirectangular(panorama, *direction(u, v)))
        face_path = destination.parent / destination.name.replace("#", str(index))
        if not face.write(Filename.fromOsSpecific(str(face_path.resolve()))):
            raise RuntimeError(f"could not write cube face {face_path}")
        print(f"prepare_sky_texture: wrote {name} face {face_path}")


def _arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="vendored tonemapped equirectangular JPG")
    parser.add_argument("destination", type=Path, help="runtime JPG to write")
    parser.add_argument("--width", type=int, default=RUNTIME_WIDTH)
    parser.add_argument("--height", type=int, default=RUNTIME_HEIGHT)
    parser.add_argument(
        "--cube-destination",
        type=Path,
        help="cube-face path template containing '#', e.g. sky_cube_#.jpg",
    )
    parser.add_argument("--cube-size", type=int, default=CUBE_FACE_SIZE)
    return parser.parse_args()


def main() -> int:
    arguments = _arguments()
    source = arguments.source.resolve()
    if not source.is_file():
        print(f"prepare_sky_texture: missing source {source}", file=sys.stderr)
        return 2

    panorama = PNMImage()
    if not panorama.read(Filename.fromOsSpecific(str(source))):
        print(f"prepare_sky_texture: could not read {source}", file=sys.stderr)
        return 2

    reduced = PNMImage(arguments.width, arguments.height, panorama.getNumChannels())
    reduced.quickFilterFrom(panorama)

    arguments.destination.parent.mkdir(parents=True, exist_ok=True)
    if not reduced.write(Filename.fromOsSpecific(str(arguments.destination.resolve()))):
        print(f"prepare_sky_texture: could not write {arguments.destination}", file=sys.stderr)
        return 2

    print(
        f"prepare_sky_texture: {panorama.getXSize()}x{panorama.getYSize()}"
        f" -> {arguments.width}x{arguments.height} {arguments.destination}"
    )

    if arguments.cube_destination is not None:
        if "#" not in arguments.cube_destination.name:
            print(
                "prepare_sky_texture: --cube-destination must contain '#'",
                file=sys.stderr,
            )
            return 2
        _write_cube_faces(reduced, arguments.cube_destination, arguments.cube_size)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
