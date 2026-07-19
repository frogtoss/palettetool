#!/usr/bin/env python3

"""Combine the palettes from one or more .pal.json documents."""

import argparse
import json
import os
from pathlib import Path
import sys
import tempfile


class PaletteDocumentError(ValueError):
    """Raised when an input is not a valid palette document."""


def parse_args():
    parser = argparse.ArgumentParser(
        description="Combine palettes from one or more .pal.json files."
    )
    parser.add_argument(
        "inputs",
        metavar="PALETTE_FILE",
        nargs="+",
        help="input .pal.json file",
    )
    parser.add_argument(
        "-o",
        "--output",
        required=True,
        help="combined output .pal.json file",
    )
    parser.add_argument(
        "--merge",
        action="store_true",
        help="keep the leftmost palette when titles conflict",
    )
    return parser.parse_args()


def reject_nonstandard_number(value):
    raise PaletteDocumentError(f"non-standard JSON number {value!r}")


def load_document(filename):
    try:
        with open(filename, "r", encoding="utf-8") as input_file:
            document = json.load(
                input_file,
                parse_constant=reject_nonstandard_number,
            )
    except json.JSONDecodeError as error:
        raise PaletteDocumentError(
            f"{filename}: invalid JSON at line {error.lineno}, "
            f"column {error.colno}: {error.msg}"
        ) from error
    except OSError as error:
        raise PaletteDocumentError(f"{filename}: {error.strerror}") from error

    if not isinstance(document, dict):
        raise PaletteDocumentError(f"{filename}: top-level value must be an object")

    palettes = document.get("palettes")
    if not isinstance(palettes, list):
        raise PaletteDocumentError(f"{filename}: 'palettes' must be an array")

    for index, palette in enumerate(palettes):
        location = f"{filename}: palette at index {index}"
        if not isinstance(palette, dict):
            raise PaletteDocumentError(f"{location} must be an object")

        title = palette.get("title")
        if not isinstance(title, str) or not title:
            raise PaletteDocumentError(
                f"{location} must have a non-empty string 'title'"
            )

    return palettes


def combine_documents(filenames, merge=False):
    combined = []
    title_sources = {}

    for filename in filenames:
        palettes = load_document(filename)
        for index, palette in enumerate(palettes):
            title = palette["title"]
            if title in title_sources:
                if merge:
                    continue
                first_filename, first_index = title_sources[title]
                raise PaletteDocumentError(
                    f"duplicate palette title {title!r} in {filename} at index "
                    f"{index}; first found in {first_filename} at index "
                    f"{first_index}"
                )

            title_sources[title] = (filename, index)
            combined.append(palette)

    return combined


def write_document(filename, palettes):
    output_path = Path(filename)
    output_directory = output_path.parent
    temporary_name = None

    try:
        file_descriptor, temporary_name = tempfile.mkstemp(
            dir=output_directory,
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
            json.dump({"palettes": palettes}, output_file, indent=4)
            output_file.write("\n")
            output_file.flush()
            os.fsync(output_file.fileno())

        os.replace(temporary_name, output_path)
        temporary_name = None
    except OSError as error:
        raise PaletteDocumentError(f"{filename}: {error.strerror}") from error
    finally:
        if temporary_name is not None:
            try:
                os.unlink(temporary_name)
            except OSError:
                pass


def main():
    args = parse_args()

    try:
        palettes = combine_documents(args.inputs, merge=args.merge)
        write_document(args.output, palettes)
    except PaletteDocumentError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    noun = "palette" if len(palettes) == 1 else "palettes"
    print(f"output {args.output} with {len(palettes)} {noun} in it")
    return 0


if __name__ == "__main__":
    sys.exit(main())
