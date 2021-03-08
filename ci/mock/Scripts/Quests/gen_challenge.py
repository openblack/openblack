#!/usr/bin/env python3

import sys
import os

def main(file_name):
    version_black_and_white = 7
    version_creature_isle = 8
    version_black_and_white_2 = 12

    with open(file_name, "wb") as f:
        # Magic number
        f.write(b"LHVM")
        # Version of the file
        f.write(version_black_and_white.to_bytes(4, byteorder='little'))
        # Number of variables
        f.write((0).to_bytes(4, byteorder='little'))
        # Number of instructions
        f.write((0).to_bytes(4, byteorder='little'))
        # Number of Auto (?)
        f.write((0).to_bytes(4, byteorder='little'))
        # Number of Scripts
        f.write((0).to_bytes(4, byteorder='little'))
        # Number of Data segments
        f.write((0).to_bytes(4, byteorder='little'))

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Generate empty Challenge Files.')
    parser.add_argument('--output-file', required=True, help='Where to generate challenge.')
    args = parser.parse_args()
    main(args.output_file)
