#!/usr/bin/env python3

import sys
import os

def main(dir_name):
    material_data = 256 * 256 * (0).to_bytes(2, byteorder='little')
    noise_map_data = 256 * 256 * (0).to_bytes(1, byteorder='little')
    bump_map_data = 256 * 256 * (0).to_bytes(1, byteorder='little')

    with open(os.path.join(dir_name, "material_data1.rgb5a1.raw"), "wb") as f:
        f.write(material_data)

    with open(os.path.join(dir_name, "material_data2.rgb5a1.raw"), "wb") as f:
        f.write(material_data)

    with open(os.path.join(dir_name, "material_data3.rgb5a1.raw"), "wb") as f:
        f.write(material_data)

    with open(os.path.join(dir_name, "noise_map.r8.raw"), "wb") as f:
        f.write(noise_map_data)

    with open(os.path.join(dir_name, "bump_map.r8.raw"), "wb") as f:
        f.write(bump_map_data)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Generate Sky Textures.')
    parser.add_argument('--output-dir', required=True, help='Where to generate textures.')
    args = parser.parse_args()
    main(args.output_dir)
