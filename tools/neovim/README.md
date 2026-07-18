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

The script captures pristine Neovim colors, loads the normal configuration to
discover its active theme, then clears and reapplies that colorscheme in
isolation. Only colors that differ from pristine Neovim are exported, excluding
unrelated defaults and plugin highlights. It writes `theme.pal.json` in the
current directory, overwrites existing output, and does not write ShaDa data.

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
direction. Background-like hints such as `selection`, `sidebar`, and `shadow`
instead select distinguishable surface colors. Every documented hint receives
at least one color; when a direct Neovim group is unavailable, semantic
fallbacks favor expected colors such as red for errors, green for success,
blue for links, and a high-contrast color for specular or cursor roles.

Lua callers can choose any output path directly after loading a colorscheme:

```lua
vim.cmd.colorscheme("habamax")
require("theme_exporter").export("/path/to/output.pal.json")
```
