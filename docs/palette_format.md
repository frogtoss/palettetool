# JSON Palette Format Documentation

This document describes the JSON format used by `palettetool` to represent and parse palettes. The format is designed to be parsed efficiently (single-pass, zero-allocation possible) and strongly typed. 

## Document Structure

A valid palette document is a single JSON object with the following properties:

- `title` *(string)*: The title or name of the palette. Max 47 characters.
- `source` *(object, optional)*: Metadata describing the origin of the palette.
  - `conversion_tool` *(string)*: The name of the tool used to convert or generate the palette.
  - `url` *(string)*: A URL related to the palette's source.
  - `conversion_date` *(string)*: A 10-base unsigned long long integer formatted as a string, representing a timestamp.
- `color_space` *(object, optional)*: Color space information for the palette.
  - `name` *(string)*: The name of the color space.
  - `icc_filename` *(string)*: The filename of an associated ICC profile.
  - `is_linear` *(boolean)*: Either `true` or `false` (no quotes), indicating if the color space is linear.
- `colors` *(array of objects, optional)*: An array defining all the colors in the palette. Must be defined *before* any other fields (like hints or gradients) reference them. Maximum of 256 colors.
- `hints` *(object, optional)*: Maps semantic meanings (hints) to arrays of color names.
- `gradients` *(object, optional)*: Maps gradient names to ordered sequences of color names.
- `dither_pairs` *(object, optional)*: Maps dither pair names to exactly two color names.
- `color_hash` *(any, optional)*: Ignored by the parser.

---

## Colors and Channels

The `colors` array defines individual colors. Each color object must contain:

- `name` *(string)*: A unique string identifier for the color.
- `red` *(float or string)*: The red channel value.
- `green` *(float or string)*: The green channel value.
- `blue` *(float or string)*: The blue channel value.
- `alpha` *(float or string)*: The alpha channel value.

### Hex Floats with Alpha Channels

Channel values (`red`, `green`, `blue`, and `alpha`) can be represented as standard decimal floats (e.g., `1.0` or `0.5`), but the **preferred** format is **hexadecimal floats** formatted as strings.

A hex float string directly encodes the underlying IEEE 754 32-bit floating-point bit representation in base-16. For example:
- `"3f800000"` represents `1.0f`.
- `"00000000"` represents `0.0f`.

The parser strictly requires each color object to have all 4 channels specified. Alpha is treated exactly like the RGB channels, supporting both standard primitive floats and hex floats in strings.

---

## Hints

Hints provide semantic meaning to the colors defined in the palette. This allows generic palettes to be used for specific themes (e.g., syntax highlighting, UI, text rendering) by hinting which colors fit best for various structural components. 

The `hints` object maps hint name strings to an array of color name strings (which must match names defined in the `colors` array).

### Available Hints

The following is the exhaustive list of valid hint strings mapped to `pal_hint_kind_t` and their expected usage:

| Hint String              | Description                                                |
|:-------------------------|:-----------------------------------------------------------|
| `"error"`                | Color for errors or critical failures.                     |
| `"warning"`              | Color for warnings.                                        |
| `"normal"`               | Default text or foreground color.                          |
| `"success"`              | Color for success states.                                  |
| `"highlight"`            | Emphasized text or UI elements.                            |
| `"urgent"`               | High-priority attention color.                             |
| `"low priority"`         | Low-priority attention color.                              |
| `"bold"`                 | Color for bolded or strong text.                           |
| `"background"`           | Main background color.                                     |
| `"background highlight"` | Slightly lighter/darker background for panels or sections. |
| `"focal point"`          | Color to draw the user's eye to a specific area.           |
| `"title"`                | Color for primary headings.                                |
| `"subtitle"`             | Color for secondary headings.                              |
| `"subsubtitle"`          | Color for tertiary headings.                               |
| `"todo"`                 | Color for TODO comments or items.                          |
| `"fixme"`                | Color for FIXME comments or items.                         |
| `"sidebar"`              | Background or foreground for sidebars.                     |
| `"subtle"`               | Muted, subdued, or disabled color.                         |
| `"shadow"`               | Color for drop shadows.                                    |
| `"specular"`             | Color for highlights or specular reflections.              |
| `"selection"`            | Background color for selected text or items.               |
| `"comment"`              | Syntax highlighting for comments.                          |
| `"string"`               | Syntax highlighting for strings.                           |
| `"keyword"`              | Syntax highlighting for keywords.                          |
| `"variable"`             | Syntax highlighting for variables.                         |
| `"operator"`             | Syntax highlighting for operators.                         |
| `"punctuation"`          | Syntax highlighting for punctuation.                       |
| `"inactive"`             | Color for disabled or inactive elements.                   |
| `"function"`             | Syntax highlighting for functions.                         |
| `"method"`               | Syntax highlighting for methods.                           |
| `"preprocessor"`         | Syntax highlighting for preprocessor directives.           |
| `"type"`                 | Syntax highlighting for types or classes.                  |
| `"constant"`             | Syntax highlighting for constants.                         |
| `"link"`                 | Color for hyperlinks.                                      |
| `"cursor"`               | Color for the text cursor or caret.                        |

---

## Gradients

The `gradients` object is used to define named multi-stop color transitions. 
- The keys in the object are the **gradient names** (max 32 total gradients).
- The values are **arrays of strings**, where each string is a color name previously defined in the `colors` array.

These arrays define the ordered stops of the gradient. 

**Example:**
```json
"gradients": {
  "sunset": ["yellow", "orange", "red", "dark_purple"]
}
```

---

## Dither Pairs

The `dither_pairs` object defines pairs of colors that work well when interleaved or dithered together to simulate an intermediate color or texture.
- The keys in the object are the **dither pair names**.
- The values are **arrays of exactly two strings**, representing two color names previously defined in the `colors` array.

If the array has more or less than two elements, the parser will fail.

**Example:**
```json
"dither_pairs": {
  "mid_gray_dither": ["black", "white"]
}
```

---

## Parse Rules & Constraints

To ensure robust loading, the parser enforces the following rules (based on `parse_json.c`):
- **Single Pass Execution:** The `colors` array must appear before `hints`, `gradients`, and `dither_pairs` since the names must be registered before they are referenced.
- **Strict Keys:** Adding undocumented keys (with the exception of `color_hash`) will result in parsing failures.
- **Length Limits:** Strings (names, URLs) are silently truncated to `PAL_MAX_STRLEN` (47 chars + null terminator).
- **Hard Limits:** Maximum of 256 colors (`PAL_MAX_COLORS`), 32 gradients (`PAL_MAX_GRADIENTS`), and 512 dither pairs (`PAL_MAX_DITHER_PAIRS`).
