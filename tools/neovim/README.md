# Neovim theme exporter

This Neovim Lua module exports the active colorscheme's RGB foreground,
background, special, and terminal palette colors to palettetool's JSON palette
format. It derives a complete set of documented semantic hints suitable for
generating themes for other editors.

Neovim 0.9 or newer is required. Run the batch exporter from the directory
where the palette should be created:

```bash
/path/to/palettetool/tools/neovim/batch_export.sh
```

The script loads the normal configuration to discover its active theme and
captures the active highlights and terminal colors before clearing them. If the
theme is available through Neovim's colorscheme completion, the exporter clears
and reapplies it in strict isolation. Reload errors are fatal. Only colors that
differ from pristine Neovim are exported, excluding unrelated defaults and
plugin highlights.

Programmatically applied active themes are also supported. These themes set
`g:colors_name` and highlights directly but have no discoverable
`colors/*.vim` or `colors/*.lua` file to reload. For such a theme, the exporter
compares its saved active state with pristine Neovim, then restores its raw
highlight definitions, links, terminal colors, and `g:colors_name`. The active
state is authoritative, so plugin-specific groups can be included when they
differ from Neovim's defaults.

The exporter writes `<theme-name>.pal.json` in the current directory, replaces
spaces in the theme name with hyphens, echoes the generated filename,
overwrites existing output, and does not write ShaDa data.

Pass an available Neovim colorscheme name to export it instead of the configured
current theme:

```bash
/path/to/palettetool/tools/neovim/batch_export.sh habamax
```

The active programmatic theme can also be passed explicitly, for example
`batch_export.sh omarchy` while Omarchy is active. An inactive programmatic
theme cannot be selected by name because Neovim has no colorscheme file with
which to load it; requesting one fails in the same way as any unavailable
colorscheme.

## Finding themes

Inside Neovim, type `:colorscheme` and press Tab to complete the themes
available on the current runtime path. The same list can be printed with:

```vim
:echo getcompletion('', 'color')
```

Use `:echo g:colors_name` to display the active theme. Themes are discovered
from `colors/*.vim` and `colors/*.lua` files on Neovim's runtime path;
`habamax` is included with Neovim.

## Color names

The output palette title is taken from `vim.g.colors_name`. Each color name is
its lowercase RGB identity, so `#1c1c1c` is named `rgb_1c1c1c`. Equal RGB
values are deduplicated, sorted by numeric RGB value, and associated with all
applicable semantic roles through the palette's `hints` object.

Channels use the palette format's preferred IEEE 754 hexadecimal float
representation and are exported as opaque sRGB colors. The exporter fails if
a theme contains more than the format's limit of 256 distinct colors.

## Hint selection

Hints use exact standard UI, syntax, diagnostic, Tree-sitter, and LSP group
names instead of broad regular expressions. Candidate colors are ranked by
semantic fit, hue, contrast, and saturation, with no more than three distinct
colors assigned to a hint.

The first `background` color determines light or dark polarity. Foreground and
syntax hints must contrast with that background and have the correct luminance
direction. Every documented hint receives at least one color; when a direct
Neovim group is unavailable, semantic fallbacks favor expected colors such as
red for errors, green for success, blue for links, and a high-contrast color
for specular or cursor roles.

After initial ranking, relational post-processing enforces the ordered theme
roles. The first `background highlight` color is a dim foreground readable on
the first `background` color. It is selected near a 30% blend toward white for
dark themes or black for light themes, with at least 2.5:1 contrast when the
theme provides a qualifying color. `selection` contains exactly two colors:
foreground first and background second. The pair uses explicit `Visual` colors
when suitable and otherwise targets at least 4.5:1 contrast. These adjustments
only select from colors already exported from the active colorscheme. The first
`bold` color is also replaced with a readable, visually distinct alternative
when it would otherwise match the first `normal` color.

Lua callers can choose any output path directly after loading a colorscheme:

```lua
vim.cmd.colorscheme("habamax")
require("theme_exporter").export("/path/to/output.pal.json")
```
