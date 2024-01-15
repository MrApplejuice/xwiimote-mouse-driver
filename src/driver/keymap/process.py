"""
This file is part of xwiimote-mouse-driver.

Foobar is free software: you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free 
Software Foundation, either version 3 of the License, or (at your option) 
any later version.

Foobar is distributed in the hope that it will be useful, but WITHOUT 
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with 
Foobar. If not, see <https://www.gnu.org/licenses/>. 
"""

import argparse
import re
from pathlib import Path

def process_keymap_file(filename: Path):
    """
    Process a keymap file and return C++ code for the keymap vector.
    """

    section_name = re.match("[0-9]+-(.*)\.txt", filename.name).group(1)
    section_name = section_name.replace("-", " ").title()

    for line in  filename.read_text().splitlines():
        if not line.strip():
            continue
        if line.startswith("#define "):
            line = line.split(" ", 1)[1]
        line = re.sub(r"\t+", "\t", line).strip()

        splits = line.split("\t")
        keyname, code = splits[:2]
        readable_name = "nullptr"
        if len(splits) > 2:
            readable_name = f'"{splits[2]}"'

        yield f'{{{keyname}, "{keyname}", {readable_name}, "{section_name}"}},'
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Process keymap files and print to stdout."
    )

    parser.add_argument("--indent", type=int, default=4)

    args = parser.parse_args()

    text_files = Path(".").glob("*.txt")
    text_files = sorted(text_files, key=lambda x: x.name)
    for filename in text_files:
        for l in process_keymap_file(filename):
            print(" " * args.indent, l)
