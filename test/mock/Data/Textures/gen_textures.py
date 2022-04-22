#!/usr/bin/env python3

import os


def import_gen_zeroed_file():
    from importlib import util
    dir_path = os.path.dirname(os.path.realpath(__file__))
    script_path = os.path.join(dir_path, "../../gen_zeroed_file.py")
    spec = util.spec_from_file_location("gen_zeroed_file", script_path)
    gen_zeroed_file = util.module_from_spec(spec)
    spec.loader.exec_module(gen_zeroed_file)
    return gen_zeroed_file


def main(dir_name):
    gen_zeroed_file = import_gen_zeroed_file()

    # R8
    gen_zeroed_file.main(os.path.join(dir_name, "smallbumpa.raw"), 256 * 256)

    # RGB8
    with open(os.path.join(dir_name, "Sky.raw"), "wb") as f:
        f.write(b'\x59\x9C\xA4' * 256 * 256)


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Generate Textures.')
    parser.add_argument('--output-dir', required=True, help='Where to generate textures.')
    args = parser.parse_args()
    main(args.output_dir)
