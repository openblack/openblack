#!/usr/bin/env python3

import os
import json
import ctypes
import struct
import base64

positions = [
    -1.0, 1.0, 0.0,
    1.0, 1.0, 0.0,
    1.0, -1.0, 0.0,
    -1.0, -1.0, 0.0,
]

uvs = [
    0.0, 1.0,
    1.0, 1.0,
    1.0, 0.0,
    0.0, 0.0,
]

normals = [
    0.0, 0.0, -1.0,
    0.0, 0.0, -1.0,
    0.0, 0.0, -1.0,
    0.0, 0.0, -1.0,
]

indices = [
    0, 1, 2,
    2, 3, 0,
]

def main(filename):

    # Create glTF mesh
    gltf_mesh = {}
    asset = {
        "generator": __file__,
        "version": "2.0"
    }
    gltf_mesh["asset"] = asset

    packed_position = struct.pack('<{}f'.format(len(positions)), *positions)
    packed_uvs = struct.pack('<{}f'.format(len(uvs)), *uvs)
    packed_normals = struct.pack('<{}f'.format(len(normals)), *normals)
    packed_indices = struct.pack('<{}H'.format(len(indices)), *indices)
    packed_buffer = packed_position + packed_normals + packed_uvs + packed_indices

    buffer = {
        "byteLength": len(packed_buffer),
        "uri": "data:application/octet-stream;base64,%s" % base64.b64encode(packed_buffer).decode()
    }
    gltf_mesh["buffers"] = [buffer]

    bufferViews = [None] * 3
    bufferViews[0] = {  # Vertex data (Position, Normals)
        "buffer": 0,
        "byteOffset": 0,
        "byteLength": len(packed_position + packed_normals),
        "byteStride": 3 * ctypes.sizeof(ctypes.c_float()),
        "target": 34962,  # ARRAY_BUFFER
    }
    bufferViews[1] = {  # Vertex data (uv)
        "buffer": 0,
        "byteOffset": bufferViews[0]["byteOffset"] + bufferViews[0]["byteLength"],
        "byteLength": len(packed_uvs),
        "target": 34962,  # ARRAY_BUFFER
    }
    bufferViews[2] = {  # Index data
        "buffer": 0,
        "byteOffset": bufferViews[1]["byteOffset"] + bufferViews[1]["byteLength"],
        "byteLength": len(packed_indices),
        "target": 34963,  # ELEMENT_ARRAY_BUFFER
    }
    gltf_mesh["bufferViews"] = bufferViews

    accessors = [None] * 4
    accessors[0] = {  # Vertex position
        "bufferView": 0,
        "byteOffset": 0,
        "componentType": 5126,  # FLOAT
        "count": len(positions) // 3,
        "type": "VEC3",
        "min": [-1.0, -1.0, 0.0],
        "max": [1.0, 1.0, 0.0],
    }
    accessors[1] = {  # Normals
        "bufferView": 0,
        "byteOffset": accessors[0]["byteOffset"] + len(positions) * ctypes.sizeof(ctypes.c_float()),
        "componentType": 5126,  # FLOAT
        "count": len(normals) // 3,
        "type": "VEC3",
        "min": [0.0, 0.0, -1.0],
        "max": [0.0, 0.0, -1.0],
    }
    accessors[2] = {  # Texture coordinates
        "bufferView": 1,
        "byteOffset": 0,
        "componentType": 5126,  # FLOAT
        "count": len(uvs) // 2,
        "type": "VEC2",
        "min": [0.0, 0.0],
        "max": [1.0, 1.0],
    }
    accessors[3] = {  # Indices
        "bufferView": 2,
        "byteOffset": 0,
        "componentType": 5123,  # UNSIGNED_SHORT
        "count": len(indices),
        "type": "SCALAR",
    }
    gltf_mesh["accessors"] = accessors

    primitive = {
        "mode": 4,  # Triangles
        "attributes": {
            "POSITION": 0,
            "TEXCOORD_0": 2,
            "NORMAL": 1,
        },
        "indices": 3,
    }
    mesh = {
        "name": "Mock Mesh",
        "primitives": [primitive],
    }
    gltf_mesh["meshes"] = [mesh]

    node = {
        "mesh": 0,
    }
    gltf_mesh["nodes"] = [node]

    gltf_mesh["scene"] = 0
    scene = {
        "nodes": [0],
    }
    gltf_mesh["scenes"] = [scene]


    with open(filename, "w") as f:
        json.dump(gltf_mesh, f, sort_keys=True, indent=4)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Generate Dummy Mesh.')
    parser.add_argument('--output-file', required=True, help='Where to generate mesh.')
    args = parser.parse_args()
    main(args.output_file)
