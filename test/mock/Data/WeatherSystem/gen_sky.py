#!/usr/bin/env python3

import os
import json
import ctypes
import struct
import base64

textures = [
    'sky_evil_day.555',
    'sky_evil_dusk.555',
    'sky_evil_night.555',
    'sky_good_day.555',
    'sky_good_dusk.555',
    'sky_good_night.555',
    'Sky_Ntrl_Day.555',
    'Sky_Ntrl_Dusk.555',
    'Sky_Ntrl_Night.555',
]
texture_width = 2
texture_height = 2


def import_gen_meshes():
    from importlib import util
    dir_path = os.path.dirname(os.path.realpath(__file__))
    script_path = os.path.join(dir_path, "../../gen_meshes.py")
    spec = util.spec_from_file_location("gen_meshes", script_path)
    gen_meshes = util.module_from_spec(spec)
    spec.loader.exec_module(gen_meshes)
    return gen_meshes


def main(dir_name):
    gen_meshes = import_gen_meshes()

    gen_meshes.main(os.path.join(dir_name, "sky.gltf"))

    for t in textures:
        with open(os.path.join(dir_name, t), "wb") as f:
            f.write((0).to_bytes(4, byteorder='little'))
            f.write(texture_width.to_bytes(4, byteorder='little'))
            f.write(texture_height.to_bytes(4, byteorder='little'))
            f.write((0).to_bytes(4, byteorder='little'))
            f.write(b'\0' * texture_width * texture_height * 2)


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Generate Sky Textures.')
    parser.add_argument('--output-dir', required=True, help='Where to generate textures.')
    args = parser.parse_args()
    main(args.output_dir)
