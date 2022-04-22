#!/usr/bin/env python3

def main(path, size):
    with open(path, "wb") as f:
        f.write(b'\x00' * size)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Generate Zeroed out file.')
    parser.add_argument('--output-file', required=True, help='Where to generate file.')
    parser.add_argument('--size', required=True, help="Size of file to generate")
    args = parser.parse_args()
    main(args.output_file, int(args.size, 0))
