#!/usr/bin/env python3

import os
import json
import ctypes
import struct
import base64

positions = [
    -1.0, 1.0, -1.0,
    1.0, 1.0, 1.0,
    1.0, -1.0, 1.0,
    -1.0, -1.0, -1.0,
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

def float3(s):
    try:
        x, y, z = map(float, s.split(','))
        return x, y, z
    except:
        raise argparse.ArgumentTypeError("Float3 must be x,y,z")

def main(output_file, translate_positions=None, scale_positions=None):
    if translate_positions is None:
        translate_positions = (0.0, 0.0, 0.0)
    if scale_positions is None:
        scale_positions = (1.0, 1.0, 1.0)

    # Create glTF mesh
    gltf_mesh = {}
    asset = {
        "generator": __file__,
        "version": "2.0"
    }
    gltf_mesh["asset"] = asset

    scaled_positions = positions
    for i in range(len(scaled_positions) // 3):
        scaled_positions[3 * i] = scaled_positions[3 * i] * scale_positions[0] + translate_positions[0]
        scaled_positions[3 * i + 1] = scaled_positions[3 * i + 1] * scale_positions[1] + translate_positions[1]
        scaled_positions[3 * i + 2] = scaled_positions[3 * i + 2] * scale_positions[2] + translate_positions[2]

    packed_position = struct.pack('<{}f'.format(len(scaled_positions)), *scaled_positions)
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

    gltf_mesh["extras"] = {
        "name": "Mock Mesh",
        "footprint": "",
        "uv2": "",
    }

    json.dump(gltf_mesh, output_file, sort_keys=True, indent=4)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Generate Dummy Mesh.')
    parser.add_argument('--translate-positions', type=float3, default=(0.0, 0.0, 0.0), help='Offset positions (for bounding box).')
    parser.add_argument('--scale-positions', type=float3, default=(1.0, 1.0, 1.0), help='Scale positions (for bounding box).')
    parser.add_argument('--output-file', type=argparse.FileType('w'), required=True, help='Where to generate mesh.')
    args = parser.parse_args()
    main(args.output_file, args.translate_positions, args.scale_positions)
