#!/usr/bin/env python3
"""Convert a skinned GLB to a static GLB while preserving its bind-pose geometry."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct


GLB_MAGIC = b"glTF"
GLB_VERSION = 2
JSON_CHUNK = 0x4E4F534A


def strip_skin(source: Path, destination: Path) -> None:
    """Remove skin declarations and unused joint attributes from a binary glTF."""

    data = source.read_bytes()
    magic, version, declared_length = struct.unpack_from("<4sII", data, 0)
    if magic != GLB_MAGIC or version != GLB_VERSION or declared_length != len(data):
        raise ValueError(f"{source} is not a valid glTF 2.0 binary")

    chunks: list[tuple[int, bytes]] = []
    offset = 12
    found_json = False
    while offset < len(data):
        chunk_length, chunk_type = struct.unpack_from("<II", data, offset)
        offset += 8
        chunk = data[offset : offset + chunk_length]
        offset += chunk_length
        if chunk_type == JSON_CHUNK:
            document = json.loads(chunk.decode("utf-8").rstrip(" \x00"))
            document.pop("skins", None)
            for node in document.get("nodes", []):
                node.pop("skin", None)
            for mesh in document.get("meshes", []):
                for primitive in mesh.get("primitives", []):
                    attributes = primitive.get("attributes", {})
                    attributes.pop("JOINTS_0", None)
                    attributes.pop("WEIGHTS_0", None)
            chunk = json.dumps(document, separators=(",", ":")).encode("utf-8")
            chunk += b" " * ((4 - len(chunk) % 4) % 4)
            found_json = True
        chunks.append((chunk_type, chunk))

    if not found_json or offset != len(data):
        raise ValueError(f"{source} contains an invalid GLB chunk table")

    output = bytearray(struct.pack("<4sII", GLB_MAGIC, GLB_VERSION, 0))
    for chunk_type, chunk in chunks:
        output.extend(struct.pack("<II", len(chunk), chunk_type))
        output.extend(chunk)
    struct.pack_into("<I", output, 8, len(output))
    destination.write_bytes(output)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    arguments = parser.parse_args()
    strip_skin(arguments.source, arguments.destination)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
