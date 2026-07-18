# JSON Palette Format Documentation

This document describes the JSON format used by `palettetool` to store one or
more palettes in a single file. Files using this format conventionally end in
`.pal.json`.

## Document Structure

A palette document is a JSON object with one top-level property:

- `palettes` *(array)*: Zero or more palette objects.

The array order is significant. Tools that select palettes by index use the
first palette as index `0`, the second as index `1`, and so on. For example,
`palettetool` selects an individual palette with `--json-palette-index`.

Palette titles are their names and should be unique within a document. Color
names and all references to them are scoped to one palette, so separate
palettes may use the same color names.

The following is a minimal document containing two palettes:

```json
{
    "palettes": [
        {
            "title": "dark example",
            "colors": [
                {
                    "name": "background",
                    "red": 0.0,
                    "green": 0.0,
                    "blue": 0.0,
                    "alpha": 1.0
                }
            ],
            "hints": {
                "background": ["background"]
            }
        },
        {
            "title": "light example",
            "colors": [
                {
                    "name": "background",
                    "red": 1.0,
                    "green": 1.0,
                    "blue": 1.0,
                    "alpha": 1.0
                }
            ],
            "hints": {
                "background": ["background"]
            }
        }
    ]
}
```

An empty palette document is represented as:

```json
{
    "palettes": []
}
```

## Palette Objects

Each element of `palettes` is an independent palette object with the following
properties:

- `title` *(string)*: The palette's name. Maximum 47 characters when read by
  the C parser.
- `source` *(object, optional)*: Metadata describing the palette's origin.
  - `conversion_tool` *(string)*: The tool used to convert or generate the
    palette.
  - `url` *(string)*: A URL related to the source.
  - `conversion_date` *(string)*: A base-10 unsigned integer stored as a string,
    usually representing a timestamp.
- `color_space` *(object, optional)*: Color-space information.
  - `name` *(string)*: The color-space name.
  - `icc_filename` *(string)*: The associated ICC profile filename.
  - `is_linear` *(boolean)*: Whether the color space is linear.
- `colors` *(array of objects, optional)*: The colors belonging to this
  palette. This property must appear before properties that refer to color
  names. Maximum 256 colors.
- `hints` *(object, optional)*: Semantic hint names mapped to arrays of color
  names.
- `gradients` *(object, optional)*: Gradient names mapped to ordered arrays of
  color names.
- `dither_pairs` *(object, optional)*: Dither-pair names mapped to arrays of
  exactly two color names.
- `color_hash` *(any JSON value, optional)*: Accepted but ignored by the C
  parser.

There is no document-level version of these properties. Metadata, colors,
hints, gradients, and dither pairs always belong to a particular element of
the `palettes` array.

## Colors and Channels

The `colors` array defines the colors available to its containing palette.
Every color object must contain:

- `name` *(string)*: A non-empty identifier unique within the palette.
- `red` *(number or string)*: The red channel value.
- `green` *(number or string)*: The green channel value.
- `blue` *(number or string)*: The blue channel value.
- `alpha` *(number or string)*: The alpha channel value.

Channel numbers are floating-point values, normally in the range `0.0` to
`1.0`. A channel may instead be encoded as a hexadecimal-float string. Such a
string contains the hexadecimal representation of the underlying IEEE 754
32-bit floating-point bits. For example:

- `"3f800000"` represents `1.0f`.
- `"00000000"` represents `0.0f`.

All four channels are required. Alpha is parsed in the same way as the red,
green, and blue channels.

## References and Scope

Hints, gradients, and dither pairs refer to colors by `name`. A reference is
resolved only against the `colors` array in the same palette object. It cannot
refer to a color in another element of the document's `palettes` array.

Because the C parser resolves names in one pass, `colors` must appear before
`hints`, `gradients`, and `dither_pairs` inside each palette object.

## Hints

Hints give semantic meaning to palette colors so that a palette can be used to
generate editor themes and other user interfaces. The `hints` object maps each
supported hint name to an ordered array of color names. Every referenced name
must exist in the same palette's `colors` array.

The following hint strings are supported:

| Hint string              | Description                                                                   |
|:-------------------------|:------------------------------------------------------------------------------|
| `"error"`                | Errors or critical failures.                                                  |
| `"warning"`              | Warnings.                                                                     |
| `"normal"`               | Default text or foreground.                                                   |
| `"success"`              | Success states.                                                               |
| `"highlight"`            | Emphasized text or UI elements.                                               |
| `"urgent"`               | High-priority attention.                                                      |
| `"low priority"`         | Low-priority attention.                                                       |
| `"bold"`                 | Bold or strong text.                                                          |
| `"background"`           | Main background.                                                              |
| `"background highlight"` | Dim foreground readable against `"background"`; the first color is preferred. |
| `"focal point"`          | A color that draws the user's eye.                                            |
| `"title"`                | Primary headings.                                                             |
| `"subtitle"`             | Secondary headings.                                                           |
| `"subsubtitle"`          | Tertiary headings.                                                            |
| `"todo"`                 | TODO comments or items.                                                       |
| `"fixme"`                | FIXME comments or items.                                                      |
| `"sidebar"`              | Sidebar foreground or background.                                             |
| `"subtle"`               | Muted, subdued, or disabled elements.                                         |
| `"shadow"`               | Drop shadows.                                                                 |
| `"specular"`             | Bright highlights or specular reflections.                                    |
| `"selection"`            | Selected-text colors, ordered foreground then background.                     |
| `"comment"`              | Syntax highlighting for comments.                                             |
| `"string"`               | Syntax highlighting for strings.                                              |
| `"keyword"`              | Syntax highlighting for keywords.                                             |
| `"variable"`             | Syntax highlighting for variables.                                            |
| `"operator"`             | Syntax highlighting for operators.                                            |
| `"punctuation"`          | Syntax highlighting for punctuation.                                          |
| `"inactive"`             | Disabled or inactive elements.                                                |
| `"function"`             | Syntax highlighting for functions.                                            |
| `"method"`               | Syntax highlighting for methods.                                              |
| `"preprocessor"`         | Syntax highlighting for preprocessor directives.                              |
| `"type"`                 | Syntax highlighting for types or classes.                                     |
| `"constant"`             | Syntax highlighting for constants.                                            |
| `"link"`                 | Hyperlinks.                                                                   |
| `"cursor"`               | Text cursor or caret.                                                         |

## Gradients

The `gradients` object defines named, ordered color transitions. Each key is a
gradient name, and each value is an array of color names from the same palette.
A palette may contain at most 32 gradients, and each gradient may contain at
most 512 color references.

```json
"gradients": {
    "sunset": ["yellow", "orange", "red", "dark purple"]
}
```

## Dither Pairs

The `dither_pairs` object defines colors that work well when interleaved to
simulate an intermediate color or texture. Each key is a dither-pair name, and
each value must contain exactly two color names from the same palette.

```json
"dither_pairs": {
    "mid gray dither": ["black", "white"]
}
```

## Parser Rules and Limits

The C parser applies these rules to every selected palette independently:

- The top-level JSON value must be an object containing a `palettes` array.

- A palette is selected by its zero-based position in that array. The command
  line option is `--json-palette-index N`.
  
- `colors` must appear before `hints`, `gradients`, and `dither_pairs` within
  each palette because color references are resolved in a single pass.
  
- Undocumented keys inside palette objects and their subobjects cause parsing
  failures, except for `color_hash`, which is ignored.
  
- Strings are silently truncated to 47 characters plus a null terminator.

- Each palette may contain at most 256 colors, 256 colors per hint, 32
  gradients, 512 references per gradient, and 512 dither pairs.
  
- A color must have a non-empty name and all four RGBA channels.

- Every color name used by a hint, gradient, or dither pair must exist in the
  same palette.

## Combining Palette Documents

`tools/combine_palettes.py` combines the `palettes` arrays from one or more
documents while preserving input-file order and palette order:

```bash
./tools/combine_palettes.py one.pal.json two.pal.json -o combined.pal.json
```

Every input must contain a `palettes` array, although that array may be empty.
Every palette must be an object with a non-empty string `title`. The command
fails without creating or replacing the output file if an input is invalid or
if any two palettes have the same title. Title comparison is case-sensitive.

## Portable Theme Hint Constraints

Hints contain ordered arrays of colors. The order implies how exporters use
them, so coherent ordering is important when a palette will generate themes
for editors or other text-oriented applications.

The parser does not enforce the following semantic rules, but palette producers
and consumers should follow them:

- Every available hint should contain at least one color. Hint arrays are
  ordered by preference, with the first color being the preferred choice.
  Exporters accessing the same hint multiple times commonly use subsequent
  colors from the same hint.

- Hints should be selective. One to three distinct colors per hint is normally
  sufficient. Semantically unrelated colors should not be included merely
  because their source names partially match a hint name.

- `background[0]` is the main background and determines theme polarity. Colors
  used as foregrounds must have the corresponding luminance direction: lighter
  than their background in dark themes and darker in light themes. They must
  also maintain enough contrast to remain readable.

- `background highlight[0]` is a dim foreground intended for use on
  `background[0]`. Its target intensity is a 30% sRGB blend from the background
  toward white for a dark theme or toward black for a light theme. Choose the
  closest existing color with the correct polarity and at least 2.5:1 contrast
  when the palette provides one; otherwise use the best-contrast existing
  color with the correct polarity.

- `bold[0]` should not equal `normal[0]` when the palette contains a suitable
  alternative. It should remain readable against `background[0]` while being
  visually distinct from `normal[0]`. Prefer another explicitly hinted bold
  color, then the closest semantically appropriate existing palette color. A
  monochrome palette may reuse `normal[0]` as a last resort.

- `selection` contains a minimum of two ordered colors: `[foreground,
  background]`. The foreground must be lighter than the selection background
  in a dark theme and darker in a light theme. Prefer a contrast ratio of at
  least 4.5:1; if the palette cannot provide that ratio, use the correctly
  ordered existing pair with the greatest available contrast.

- When a direct semantic color is unavailable, fallbacks should follow common
  theme conventions: red for errors, urgent states, and FIXME markers; yellow
  or orange for warnings and TODO markers; green for success and strings; blue
  or cyan for links, functions, methods, and types; muted readable colors for
  comments and low-priority states; and high-contrast colors for cursors, focal
  points, and specular highlights.
