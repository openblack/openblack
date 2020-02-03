#!/usr/bin/env python3

import sys
import os

def main(dir_name):
    # R8
    with open(os.path.join(dir_name, "smallbumpa.raw"), "wb") as f:
        f.write(b'\x00' * 256 * 256)

    # RGB8
    with open(os.path.join(dir_name, "Sky.raw"), "wb") as f:
        f.write(b'\x59\x9C\xA4' * 256 * 256)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Generate Textures.')
    parser.add_argument('--output-dir', required=True, help='Where to generate textures.')
    args = parser.parse_args()
    main(args.output_dir)
