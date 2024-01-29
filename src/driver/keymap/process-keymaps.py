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

import argparse
import re
from pathlib import Path


def keymap_to_py(filename: Path):
    """
    Process a keymap file and return C++ code for the keymap vector.
    """

    section_name = re.match("[0-9]+-(.*)\.txt", filename.name).group(1)
    section_name = section_name.replace("-", " ").title()

    template = r"""
{
    "keycode": %CODE%,
    "keyname": "%KEYNAME%",
    "readable_name": %READABLE_NAME%,
    "section_name": "%SECTION_NAME%"
},
    """.strip()

    for line in filename.read_text().splitlines():
        if not line.strip():
            continue
        if line.startswith("#define "):
            line = line.split(" ", 1)[1]
        line = re.sub(r"\t+", "\t", line).strip()

        splits = line.split("\t")
        keyname, code = splits[:2]
        readable_name = "None"
        if len(splits) > 2:
            readable_name = f'"{splits[2]}"'

        for l in template.splitlines():
            l = l.replace("%CODE%", code)
            l = l.replace("%KEYNAME%", keyname)
            l = l.replace("%READABLE_NAME%", readable_name)
            l = l.replace("%SECTION_NAME%", section_name)
            yield l


def keymap_to_cpp(filename: Path):
    """
    Process a keymap file and return C++ code for the keymap vector.
    """

    section_name = re.match("[0-9]+-(.*)\.txt", filename.name).group(1)
    section_name = section_name.replace("-", " ").title()

    for line in filename.read_text().splitlines():
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
    g = parser.add_mutually_exclusive_group(required=True)
    g.add_argument("--cpp", action="store_true", help="Print C++ code.")
    g.add_argument("--py", action="store_true", help="Print Python code.")

    args = parser.parse_args()

    text_files = Path(".").glob("*.txt")
    text_files = sorted(text_files, key=lambda x: x.name)

    if args.cpp:
        converter = keymap_to_cpp
    if args.py:
        converter = keymap_to_py

    for filename in text_files:
        for l in converter(filename):
            print(" " * args.indent, l)
