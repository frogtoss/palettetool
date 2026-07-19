#!/usr/bin/env python3

"""Import Omarchy colors.toml files into a palette document."""

import argparse
import json
import os
from pathlib import Path
import re
import struct
import sys
import tempfile
import tomllib


MAX_COLORS = 256
MAX_STRING_BYTES = 47
HEX_COLOR_PATTERN = re.compile(r"#[0-9A-Fa-f]{6}\Z")


class ImportColorsError(ValueError):
    """Raised when an Omarchy theme cannot be imported."""


def f32_to_hex(value):
    return struct.pack(">f", value).hex()


def parse_args():
    parser = argparse.ArgumentParser(
        description="Import Omarchy colors.toml files into a .pal.json file."
    )
    parser.add_argument(
        "inputs",
        metavar="COLORS_TOML",
        nargs="+",
        help="input Omarchy colors.toml file",
    )
    parser.add_argument(
        "-o",
        "--output",
        required=True,
        help="output .pal.json file",
    )
    return parser.parse_args()


def check_string_limit(value, description, filename):
    byte_count = len(value.encode("utf-8"))
    if byte_count > MAX_STRING_BYTES:
        raise ImportColorsError(
            f"{filename}: {description} exceeds the "
            f"{MAX_STRING_BYTES}-byte limit"
        )


def load_colors(filename):
    try:
        with open(filename, "rb") as input_file:
            colors = tomllib.load(input_file)
    except (tomllib.TOMLDecodeError, UnicodeDecodeError) as error:
        raise ImportColorsError(f"{filename}: invalid TOML: {error}") from error
    except OSError as error:
        raise ImportColorsError(f"{filename}: {error.strerror}") from error

    if not colors:
        raise ImportColorsError(f"{filename}: palette is empty")
    if len(colors) > MAX_COLORS:
        raise ImportColorsError(
            f"{filename}: palette has {len(colors)} colors; "
            f"maximum is {MAX_COLORS}"
        )

    parsed_colors = []
    for name, value in colors.items():
        if not name:
            raise ImportColorsError(f"{filename}: color name must not be empty")
        check_string_limit(name, f"color name {name!r}", filename)

        if isinstance(value, dict):
            raise ImportColorsError(
                f"{filename}: nested value for color {name!r} is not allowed"
            )
        if not isinstance(value, str):
            raise ImportColorsError(
                f"{filename}: value for color {name!r} must be a string"
            )
        if HEX_COLOR_PATTERN.fullmatch(value) is None:
            raise ImportColorsError(
                f"{filename}: color {name!r} must be in #RRGGBB format"
            )

        parsed_colors.append(
            {
                "name": name,
                "red": f32_to_hex(int(value[1:3], 16) / 255.0),
                "green": f32_to_hex(int(value[3:5], 16) / 255.0),
                "blue": f32_to_hex(int(value[5:7], 16) / 255.0),
                "alpha": f32_to_hex(1.0),
            }
        )

    return parsed_colors


def unique_title(filename, used_titles):
    base_title = Path(filename).absolute().parent.name
    if not base_title:
        raise ImportColorsError(f"{filename}: cannot derive a palette title")

    title = base_title
    suffix = 2
    while title in used_titles:
        title = f"{base_title} {suffix}"
        suffix += 1

    check_string_limit(title, f"palette title {title!r}", filename)
    used_titles.add(title)
    return title


def import_palettes(filenames):
    palettes = []
    used_titles = set()

    for filename in filenames:
        colors = load_colors(filename)
        palettes.append(
            {
                "title": unique_title(filename, used_titles),
                "colors": colors,
                "color_space": {
                    "name": "sRGB",
                    "is_linear": False,
                },
            }
        )

    return palettes


def write_document(filename, palettes):
    output_path = Path(filename)
    temporary_name = None

    try:
        file_descriptor, temporary_name = tempfile.mkstemp(
            dir=output_path.parent,
            prefix=f".{output_path.name}.",
            suffix=".tmp",
            text=True,
        )
        with os.fdopen(
            file_descriptor,
            "w",
            encoding="utf-8",
            newline="\n",
        ) as output_file:
            json.dump(
                {"palettes": palettes},
                output_file,
                indent=4,
                ensure_ascii=False,
            )
            output_file.write("\n")
            output_file.flush()
            os.fsync(output_file.fileno())

        os.replace(temporary_name, output_path)
        temporary_name = None
    except OSError as error:
        raise ImportColorsError(f"{filename}: {error.strerror}") from error
    finally:
        if temporary_name is not None:
            try:
                os.unlink(temporary_name)
            except OSError:
                pass


def main():
    args = parse_args()

    try:
        palettes = import_palettes(args.inputs)
        write_document(args.output, palettes)
    except ImportColorsError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    noun = "palette" if len(palettes) == 1 else "palettes"
    print(f"output {args.output} with {len(palettes)} {noun}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
