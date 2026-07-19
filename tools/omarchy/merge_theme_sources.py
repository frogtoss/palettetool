#!/usr/bin/env python3

"""Merge TOML-derived and Neovim-derived Omarchy palettes."""

import argparse
import json
import os
from pathlib import Path
import re
import sys
import tempfile


MAX_COLORS = 256
MAX_STRING_BYTES = 47
HEX_FLOAT_PATTERN = re.compile(r"[0-9A-Fa-f]{8}\Z")
INDEXED_COLOR_PATTERN = re.compile(r"color(0|[1-9][0-9]*)\Z")
CHANNEL_NAMES = ("red", "green", "blue", "alpha")
SUPPORTED_HINTS = frozenset(
    {
        "error",
        "warning",
        "normal",
        "success",
        "highlight",
        "urgent",
        "low priority",
        "bold",
        "background",
        "background highlight",
        "focal point",
        "title",
        "subtitle",
        "subsubtitle",
        "todo",
        "fixme",
        "sidebar",
        "subtle",
        "shadow",
        "specular",
        "selection",
        "comment",
        "string",
        "keyword",
        "variable",
        "operator",
        "punctuation",
        "inactive",
        "function",
        "method",
        "preprocessor",
        "type",
        "constant",
        "link",
        "cursor",
    }
)


class MergeThemeError(ValueError):
    """Raised when theme palettes cannot be merged."""


def parse_args():
    parser = argparse.ArgumentParser(
        description="Merge TOML-derived and Neovim-derived Omarchy palettes."
    )
    parser.add_argument(
        "--from-toml",
        required=True,
        metavar="PALETTE_FILE",
        help="palette generated from Omarchy colors.toml",
    )
    parser.add_argument(
        "--from-neovim",
        required=True,
        metavar="PALETTE_FILE",
        help="palette generated from the Neovim theme",
    )
    parser.add_argument(
        "-o",
        "--output",
        required=True,
        help="merged output .pal.json file",
    )
    return parser.parse_args()


def reject_nonstandard_number(value):
    raise MergeThemeError(f"non-standard JSON number {value!r}")


def check_string(value, description, filename):
    if not isinstance(value, str) or not value:
        raise MergeThemeError(
            f"{filename}: {description} must be a non-empty string"
        )

    try:
        byte_count = len(value.encode("utf-8"))
    except UnicodeEncodeError as error:
        raise MergeThemeError(
            f"{filename}: {description} must be valid UTF-8"
        ) from error

    if byte_count > MAX_STRING_BYTES:
        raise MergeThemeError(
            f"{filename}: {description} exceeds the "
            f"{MAX_STRING_BYTES}-byte limit"
        )


def load_document(filename):
    try:
        with open(filename, "r", encoding="utf-8") as input_file:
            document = json.load(
                input_file,
                parse_constant=reject_nonstandard_number,
            )
    except json.JSONDecodeError as error:
        raise MergeThemeError(
            f"{filename}: invalid JSON at line {error.lineno}, "
            f"column {error.colno}: {error.msg}"
        ) from error
    except MergeThemeError as error:
        raise MergeThemeError(f"{filename}: {error}") from error
    except UnicodeDecodeError as error:
        raise MergeThemeError(f"{filename}: invalid UTF-8") from error
    except OSError as error:
        raise MergeThemeError(f"{filename}: {error.strerror}") from error

    if not isinstance(document, dict):
        raise MergeThemeError(f"{filename}: top-level value must be an object")

    palettes = document.get("palettes")
    if not isinstance(palettes, list):
        raise MergeThemeError(f"{filename}: 'palettes' must be an array")

    return palettes


def validate_color_space(color_space, filename, location):
    if not isinstance(color_space, dict):
        raise MergeThemeError(f"{filename}: {location} must be an object")

    allowed_keys = {"name", "icc_filename", "is_linear"}
    unknown_keys = color_space.keys() - allowed_keys
    if unknown_keys:
        unknown_key = sorted(unknown_keys)[0]
        raise MergeThemeError(
            f"{filename}: {location} has unknown field {unknown_key!r}"
        )

    normalized = {}
    for key, value in color_space.items():
        if key == "is_linear":
            if not isinstance(value, bool):
                raise MergeThemeError(
                    f"{filename}: {location}.{key} must be a boolean"
                )
        else:
            check_string(value, f"{location}.{key}", filename)
        normalized[key] = value

    return normalized


def validate_colors(colors, filename, location):
    if not isinstance(colors, list):
        raise MergeThemeError(f"{filename}: {location} must be an array")
    if len(colors) > MAX_COLORS:
        raise MergeThemeError(
            f"{filename}: {location} has {len(colors)} colors; "
            f"maximum is {MAX_COLORS}"
        )

    normalized = []
    names = set()
    for index, color in enumerate(colors):
        color_location = f"{location}[{index}]"
        if not isinstance(color, dict):
            raise MergeThemeError(
                f"{filename}: {color_location} must be an object"
            )

        name = color.get("name")
        check_string(name, f"{color_location}.name", filename)
        if name in names:
            raise MergeThemeError(
                f"{filename}: duplicate color name {name!r}"
            )
        names.add(name)

        normalized_color = {"name": name}
        for channel_name in CHANNEL_NAMES:
            channel = color.get(channel_name)
            if (
                not isinstance(channel, str)
                or HEX_FLOAT_PATTERN.fullmatch(channel) is None
            ):
                raise MergeThemeError(
                    f"{filename}: {color_location}.{channel_name} must be "
                    "an eight-digit hexadecimal-float string"
                )
            normalized_color[channel_name] = channel.lower()

        normalized.append(normalized_color)

    return normalized, names


def validate_hints(hints, color_names, filename, location):
    if hints is None:
        return {}
    if not isinstance(hints, dict):
        raise MergeThemeError(f"{filename}: {location} must be an object")

    normalized = {}
    for hint_name, references in hints.items():
        if hint_name not in SUPPORTED_HINTS:
            raise MergeThemeError(
                f"{filename}: unsupported hint {hint_name!r}"
            )
        if not isinstance(references, list):
            raise MergeThemeError(
                f"{filename}: {location}.{hint_name} must be an array"
            )
        if len(references) > MAX_COLORS:
            raise MergeThemeError(
                f"{filename}: {location}.{hint_name} has more than "
                f"{MAX_COLORS} color references"
            )

        normalized_references = []
        for reference in references:
            if not isinstance(reference, str) or reference not in color_names:
                raise MergeThemeError(
                    f"{filename}: hint {hint_name!r} references unknown "
                    f"color {reference!r}"
                )
            normalized_references.append(reference)
        normalized[hint_name] = normalized_references

    return normalized


def validate_palette(palette, filename, palette_index, role):
    location = f"palette at index {palette_index} ({role})"
    if not isinstance(palette, dict):
        raise MergeThemeError(f"{filename}: {location} must be an object")

    title = palette.get("title")
    check_string(title, f"{location}.title", filename)
    colors, color_names = validate_colors(
        palette.get("colors"),
        filename,
        f"{location}.colors",
    )
    hints = validate_hints(
        palette.get("hints"),
        color_names,
        filename,
        f"{location}.hints",
    )

    color_space = None
    if "color_space" in palette:
        color_space = validate_color_space(
            palette["color_space"],
            filename,
            f"{location}.color_space",
        )

    return {
        "title": title,
        "colors": colors,
        "hints": hints,
        "color_space": color_space,
    }


def load_source_palettes(toml_filename, neovim_filename):
    same_file = Path(toml_filename).resolve() == Path(neovim_filename).resolve()
    if same_file:
        palettes = load_document(toml_filename)
        if len(palettes) != 2:
            raise MergeThemeError(
                f"{toml_filename}: expected exactly 2 palettes when used "
                f"for both inputs; found {len(palettes)}"
            )
        return (
            validate_palette(palettes[0], toml_filename, 0, "from-toml"),
            validate_palette(palettes[1], toml_filename, 1, "from-neovim"),
        )

    toml_palettes = load_document(toml_filename)
    if len(toml_palettes) != 1:
        raise MergeThemeError(
            f"{toml_filename}: expected exactly 1 from-toml palette; "
            f"found {len(toml_palettes)}"
        )

    neovim_palettes = load_document(neovim_filename)
    if len(neovim_palettes) != 1:
        raise MergeThemeError(
            f"{neovim_filename}: expected exactly 1 from-neovim palette; "
            f"found {len(neovim_palettes)}"
        )

    return (
        validate_palette(toml_palettes[0], toml_filename, 0, "from-toml"),
        validate_palette(neovim_palettes[0], neovim_filename, 0, "from-neovim"),
    )


def indexed_color_number(name):
    match = INDEXED_COLOR_PATTERN.fullmatch(name)
    if match is None:
        return None
    return int(match.group(1))


def order_toml_colors(colors, filename):
    indexed = []
    unindexed = []
    for color in colors:
        color_index = indexed_color_number(color["name"])
        if color_index is None:
            unindexed.append(color)
        else:
            indexed.append((color_index, color))

    indexed.sort(key=lambda item: item[0])
    actual_indices = [item[0] for item in indexed]
    if actual_indices and actual_indices != list(range(actual_indices[-1] + 1)):
        raise MergeThemeError(
            f"{filename}: indexed colors must form a contiguous sequence "
            "starting at color0"
        )

    return [item[1] for item in indexed] + unindexed


def color_channels(color):
    return tuple(color[channel_name] for channel_name in CHANNEL_NAMES)


def choose_toml_match(matches):
    indexed = []
    for input_index, color in matches:
        color_index = indexed_color_number(color["name"])
        if color_index is not None:
            indexed.append((color_index, color))

    if indexed:
        return min(indexed, key=lambda item: item[0])[1]
    return min(matches, key=lambda item: item[0])[1]


def deduplicate_neovim_colors(toml_colors, neovim_colors):
    toml_colors_by_channels = {}
    for input_index, color in enumerate(toml_colors):
        channels = color_channels(color)
        toml_colors_by_channels.setdefault(channels, []).append(
            (input_index, color)
        )

    replacements = {}
    survivors = []
    toml_names = {color["name"] for color in toml_colors}
    for color in neovim_colors:
        matches = toml_colors_by_channels.get(color_channels(color))
        if matches:
            replacements[color["name"]] = choose_toml_match(matches)["name"]
            continue

        if color["name"] in toml_names:
            raise MergeThemeError(
                f"surviving Neovim color name {color['name']!r} conflicts "
                "with a TOML color"
            )
        replacements[color["name"]] = color["name"]
        survivors.append(color)

    return survivors, replacements


def stable_unique(values):
    result = []
    seen = set()
    for value in values:
        if value not in seen:
            seen.add(value)
            result.append(value)
    return result


def put_hint_first(hints, hint_name, color_name, final_names):
    if color_name not in final_names:
        return
    references = [
        reference
        for reference in hints.get(hint_name, [])
        if reference != color_name
    ]
    hints[hint_name] = [color_name] + references


def fixup_hints(hints, final_names):
    put_hint_first(hints, "background", "background", final_names)
    put_hint_first(hints, "cursor", "cursor", final_names)

    selection_foreground = "selection_foreground"
    selection_background = "selection_background"
    if (
        selection_foreground in final_names
        and selection_background in final_names
    ):
        references = [
            reference
            for reference in hints.get("selection", [])
            if reference not in {selection_foreground, selection_background}
        ]
        hints["selection"] = [
            selection_foreground,
            selection_background,
        ] + references

    put_hint_first(hints, "normal", "foreground", final_names)


def limit_positional_hints(hints):
    for hint_name in ("selection", "cursor"):
        if hint_name in hints:
            hints[hint_name] = hints[hint_name][:2]


def merge_hints(toml_hints, neovim_hints, replacements, final_names):
    merged = {
        hint_name: stable_unique(references)
        for hint_name, references in toml_hints.items()
    }

    for hint_name, references in neovim_hints.items():
        mapped = [replacements[reference] for reference in references]
        merged[hint_name] = stable_unique(merged.get(hint_name, []) + mapped)

    fixup_hints(merged, final_names)
    limit_positional_hints(merged)

    for hint_name, references in merged.items():
        if len(references) != len(set(references)):
            raise MergeThemeError(
                f"internal error: hint {hint_name!r} contains duplicates"
            )
        for reference in references:
            if reference not in final_names:
                raise MergeThemeError(
                    f"internal error: hint {hint_name!r} references "
                    f"unknown color {reference!r}"
                )

    return merged


def merge_palettes(toml_palette, neovim_palette, toml_filename):
    ordered_toml_colors = order_toml_colors(
        toml_palette["colors"],
        toml_filename,
    )
    surviving_neovim_colors, replacements = deduplicate_neovim_colors(
        toml_palette["colors"],
        neovim_palette["colors"],
    )
    colors = ordered_toml_colors + surviving_neovim_colors
    if len(colors) > MAX_COLORS:
        raise MergeThemeError(
            f"merged palette has {len(colors)} colors; maximum is {MAX_COLORS}"
        )

    final_names = {color["name"] for color in colors}
    hints = merge_hints(
        toml_palette["hints"],
        neovim_palette["hints"],
        replacements,
        final_names,
    )

    title = f"{toml_palette['title']} merged"
    check_string(title, "merged palette title", toml_filename)

    merged = {
        "title": title,
        "colors": colors,
    }
    if toml_palette["color_space"] is not None:
        merged["color_space"] = toml_palette["color_space"]
    merged["hints"] = hints
    return merged


def write_document(filename, palette):
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
                {"palettes": [palette]},
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
        raise MergeThemeError(f"{filename}: {error.strerror}") from error
    finally:
        if temporary_name is not None:
            try:
                os.unlink(temporary_name)
            except OSError:
                pass


def main():
    args = parse_args()

    try:
        toml_palette, neovim_palette = load_source_palettes(
            args.from_toml,
            args.from_neovim,
        )
        merged = merge_palettes(
            toml_palette,
            neovim_palette,
            args.from_toml,
        )
        write_document(args.output, merged)
    except MergeThemeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    print(
        f"output {args.output} with merged palette {merged['title']!r} "
        f"({len(merged['colors'])} colors)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
