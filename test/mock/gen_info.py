#!/usr/bin/env python3

import gen_zeroed_file

def main(path):
    # Set file as zeroed
    size = 0x8E186
    gen_zeroed_file.main(path, size)

    # Add abode name used in scene test
    abode_debug_name = b'ABODE_F'
    abode_info_array_offset = 0x12bf8 # 0x1b8 long
    abode_debug_string_offset = 8  # 0x30 long
    with open(path, "r+b") as f:
        f.seek(abode_info_array_offset + abode_debug_string_offset)
        f.write(abode_debug_name)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Generate Mock info.dat file.')
    parser.add_argument('--output-file', required=True, help='Where to generate file.')
    args = parser.parse_args()
    main(args.output_file)
