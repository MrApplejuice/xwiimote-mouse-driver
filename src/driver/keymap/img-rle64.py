"""
This file is part of xwiimote-mouse-driver.

xwiimote-mouse-driver is free software: you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free 
Software Foundation, either version 3 of the License, or (at your option) 
any later version.

xwiimote-mouse-driver is distributed in the hope that it will be useful, but WITHOUT 
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with 
xwiimote-mouse-driver. If not, see <https://www.gnu.org/licenses/>. 
"""

import sys
import argparse
from PIL import Image
import numpy as np
import struct
import lzma
import base64


def main():
    parser = argparse.ArgumentParser(description="Convert images to rle64 format.")

    parser.add_argument("image", type=str, help="Image to convert.")
    parser.add_argument("--length", type=int, default=None, help="Maximum line length.")
    parser.add_argument(
        "--pyraw", action="store_true", help="Output python raw strings code."
    )

    args = parser.parse_args()

    img = Image.open(args.image)
    imgdata = np.asarray(img)
    height, width, ncolors = imgdata.shape

    rle_data = []
    backrefs = []
    last_sample = None
    for y in range(height):
        for x in range(width):
            sample = imgdata[y][x].tolist()
            if sample == last_sample:
                rle_data[-1][0] += 1
            elif sample in backrefs:
                rle_data.append([1, backrefs.index(sample)])
                last_sample = sample
            else:
                rle_data.append([1, sample])
                backrefs.append(sample)
                last_sample = sample

    if len(backrefs) > 0xFFFF:
        raise Exception("Too many backrefs.")

    binary_rle_data = struct.pack("<HHH", width, height, ncolors)
    for reps, sample in rle_data:
        if isinstance(sample, int):
            binary_rle_data += struct.pack("<lH", -reps, sample)
        else:
            binary_rle_data += struct.pack("<l" + "B" * ncolors, reps, *sample)

    lzma_data = lzma.compress(binary_rle_data, preset=9)
    b85data = base64.b85encode(lzma_data).decode("ascii")
    total_size = len(b85data)

    if args.length is not None:
        b85data = [
            b85data[i : i + args.length] for i in range(0, len(b85data), args.length)
        ]
    else:
        b85data = [b85data]

    for line in b85data:
        if args.pyraw:
            line = f'r"{line}"'
        print(line)

    print(f"Total size: {total_size}", file=sys.stderr)
    print(f"Number of backrefs: {len(backrefs)}", file=sys.stderr)


if __name__ == "__main__":
    main()
