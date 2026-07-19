local M = {}

local MAX_COLORS = 256
local MAX_HINT_COLORS = 3
local MAX_STRING_LENGTH = 47
local ATTRIBUTES = { "fg", "bg", "sp" }

local HINT_ORDER = {
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

-- Sources are exact Neovim highlight group and attribute pairs, ordered from
-- the strongest semantic match to weaker alternatives.  Broad name regexes
-- intentionally are not used: they make plugin group names pollute unrelated
-- hints (most noticeably the "link" hint).
local HINT_PROFILES = {
    error = {
        kind = "accent",
        hue = 0.0,
        sources = {
            { "DiagnosticError", "fg", 120 },
            { "ErrorMsg", "fg", 115 },
            { "Error", "fg", 110 },
            { "SpellBad", "sp", 100 },
            { "@comment.error", "fg", 95 },
        },
    },
    warning = {
        kind = "accent",
        hue = 0.12,
        sources = {
            { "DiagnosticWarn", "fg", 120 },
            { "WarningMsg", "fg", 115 },
            { "SpellCap", "sp", 100 },
            { "@comment.warning", "fg", 95 },
        },
    },
    normal = {
        kind = "foreground",
        neutral = true,
        min_contrast = 4.0,
        max_colors = 1,
        sources = { { "Normal", "fg", 150 } },
    },
    success = {
        kind = "accent",
        hue = 0.33,
        sources = {
            { "DiagnosticOk", "fg", 120 },
            { "Added", "fg", 110 },
            { "DiffAdd", "fg", 105 },
            { "MoreMsg", "fg", 90 },
        },
    },
    highlight = {
        kind = "accent",
        sources = {
            { "Search", "fg", 120 },
            { "CurSearch", "fg", 115 },
            { "IncSearch", "fg", 110 },
            { "MatchParen", "fg", 100 },
            { "CursorLineNr", "fg", 95 },
        },
    },
    urgent = {
        kind = "bright",
        hue = 0.0,
        max_colors = 1,
        sources = {
            { "ErrorMsg", "fg", 130 },
            { "DiagnosticError", "fg", 120 },
        },
    },
    ["low priority"] = {
        kind = "muted",
        min_contrast = 2.0,
        sources = {
            { "NonText", "fg", 120 },
            { "LineNr", "fg", 115 },
            { "Conceal", "fg", 105 },
            { "LspCodeLens", "fg", 100 },
        },
    },
    bold = {
        kind = "strong",
        sources = {
            { "Title", "fg", 120 },
            { "Statement", "fg", 110 },
            { "Todo", "fg", 100 },
        },
        include_bold_groups = true,
    },
    background = {
        kind = "background",
        max_colors = 1,
        sources = { { "Normal", "bg", 200 } },
    },
    ["background highlight"] = {
        kind = "surface",
        target_contrast = 1.3,
        sources = {
            { "CursorLine", "bg", 130 },
            { "ColorColumn", "bg", 120 },
            { "Pmenu", "bg", 110 },
            { "NormalFloat", "bg", 100 },
        },
    },
    ["focal point"] = {
        kind = "bright",
        sources = {
            { "MatchParen", "fg", 120 },
            { "CursorLineNr", "fg", 115 },
            { "Search", "fg", 105 },
        },
    },
    title = {
        kind = "strong",
        sources = {
            { "Title", "fg", 130 },
            { "@markup.heading", "fg", 120 },
            { "@markup.heading.1", "fg", 115 },
            { "FloatTitle", "fg", 100 },
        },
    },
    subtitle = {
        kind = "strong",
        sources = {
            { "@markup.heading.2", "fg", 125 },
            { "WinBar", "fg", 105 },
            { "Title", "fg", 90 },
        },
    },
    subsubtitle = {
        kind = "strong",
        sources = {
            { "@markup.heading.3", "fg", 125 },
            { "@markup.heading.2", "fg", 100 },
            { "Title", "fg", 90 },
        },
    },
    todo = {
        kind = "accent",
        hue = 0.12,
        sources = {
            { "Todo", "fg", 130 },
            { "@comment.todo", "fg", 120 },
            { "DiagnosticWarn", "fg", 95 },
        },
    },
    fixme = {
        kind = "accent",
        hue = 0.0,
        sources = {
            { "Fixme", "fg", 130 },
            { "FIXME", "fg", 130 },
            { "DiagnosticError", "fg", 110 },
            { "ErrorMsg", "fg", 100 },
        },
    },
    sidebar = {
        kind = "surface",
        target_contrast = 1.15,
        sources = {
            { "NeoTreeNormal", "bg", 130 },
            { "NvimTreeNormal", "bg", 130 },
            { "SignColumn", "bg", 110 },
            { "FoldColumn", "bg", 105 },
            { "NormalFloat", "bg", 90 },
        },
    },
    subtle = {
        kind = "muted",
        min_contrast = 2.0,
        sources = {
            { "NonText", "fg", 125 },
            { "Whitespace", "fg", 120 },
            { "Comment", "fg", 100 },
            { "LineNr", "fg", 95 },
        },
    },
    shadow = {
        kind = "shadow",
        max_colors = 1,
        sources = {
            { "FloatShadow", "bg", 130 },
            { "FloatShadowThrough", "bg", 120 },
        },
    },
    specular = {
        kind = "bright",
        max_colors = 1,
        sources = {
            { "Cursor", "bg", 110 },
            { "CurSearch", "fg", 105 },
            { "DiagnosticInfo", "fg", 95 },
        },
    },
    selection = {
        kind = "surface",
        target_contrast = 1.8,
        sources = {
            { "Visual", "bg", 140 },
            { "PmenuSel", "bg", 120 },
            { "LspReferenceText", "bg", 110 },
        },
    },
    comment = {
        kind = "muted",
        min_contrast = 2.0,
        sources = {
            { "Comment", "fg", 140 },
            { "@comment", "fg", 130 },
            { "@comment.documentation", "fg", 110 },
        },
    },
    string = {
        kind = "accent",
        hue = 0.33,
        sources = {
            { "String", "fg", 140 },
            { "@string", "fg", 130 },
            { "Character", "fg", 110 },
            { "@character", "fg", 100 },
        },
    },
    keyword = {
        kind = "accent",
        hue = 0.78,
        sources = {
            { "Keyword", "fg", 140 },
            { "@keyword", "fg", 130 },
            { "Statement", "fg", 110 },
            { "Conditional", "fg", 100 },
        },
    },
    variable = {
        kind = "foreground",
        sources = {
            { "@variable", "fg", 140 },
            { "Identifier", "fg", 125 },
            { "@variable.member", "fg", 115 },
            { "@variable.parameter", "fg", 110 },
        },
    },
    operator = {
        kind = "accent",
        hue = 0.5,
        sources = {
            { "Operator", "fg", 140 },
            { "@operator", "fg", 130 },
        },
    },
    punctuation = {
        kind = "muted",
        min_contrast = 2.5,
        sources = {
            { "Delimiter", "fg", 140 },
            { "@punctuation.delimiter", "fg", 130 },
            { "@punctuation.bracket", "fg", 125 },
            { "@punctuation", "fg", 110 },
        },
    },
    inactive = {
        kind = "muted",
        min_contrast = 2.0,
        sources = {
            { "StatusLineNC", "fg", 130 },
            { "WinBarNC", "fg", 120 },
            { "TabLine", "fg", 110 },
            { "NormalNC", "fg", 100 },
        },
    },
    ["function"] = {
        kind = "accent",
        hue = 0.58,
        sources = {
            { "Function", "fg", 140 },
            { "@function", "fg", 130 },
            { "@function.call", "fg", 120 },
        },
    },
    method = {
        kind = "accent",
        hue = 0.58,
        sources = {
            { "@function.method", "fg", 140 },
            { "@function.method.call", "fg", 130 },
            { "@lsp.type.method", "fg", 120 },
            { "Function", "fg", 90 },
        },
    },
    preprocessor = {
        kind = "accent",
        hue = 0.78,
        sources = {
            { "PreProc", "fg", 140 },
            { "Macro", "fg", 130 },
            { "@constant.macro", "fg", 125 },
            { "@keyword.directive", "fg", 120 },
        },
    },
    type = {
        kind = "accent",
        hue = 0.5,
        sources = {
            { "Type", "fg", 140 },
            { "@type", "fg", 130 },
            { "@type.builtin", "fg", 120 },
            { "Structure", "fg", 100 },
        },
    },
    constant = {
        kind = "accent",
        hue = 0.08,
        sources = {
            { "Constant", "fg", 140 },
            { "@constant", "fg", 130 },
            { "Number", "fg", 120 },
            { "Boolean", "fg", 110 },
        },
    },
    link = {
        kind = "accent",
        hue = 0.58,
        sources = {
            { "Underlined", "fg", 140 },
            { "@markup.link", "fg", 130 },
            { "@markup.link.url", "fg", 125 },
            { "@string.special.url", "fg", 120 },
        },
    },
    cursor = {
        kind = "bright",
        max_colors = 1,
        sources = {
            { "Cursor", "bg", 150 },
            { "lCursor", "bg", 140 },
            { "CursorIM", "bg", 130 },
            { "TermCursor", "bg", 120 },
        },
    },
}

local function json_string(value)
    if vim.json and vim.json.encode then
        return vim.json.encode(value)
    end
    return vim.fn.json_encode(value)
end

local function truncate_string(value)
    value = tostring(value)
    while #value > MAX_STRING_LENGTH do
        local character_count = vim.fn.strchars(value)
        value = vim.fn.strcharpart(value, 0, character_count - 1)
    end
    return value
end

local function float32_hex(value)
    if value == 0 then
        return "00000000"
    end

    local mantissa, exponent = math.frexp(value)
    local biased_exponent = exponent + 126
    local fraction = math.floor(((mantissa * 2 - 1) * 2 ^ 23) + 0.5)

    if fraction == 2 ^ 23 then
        fraction = 0
        biased_exponent = biased_exponent + 1
    end

    return string.format("%08x", biased_exponent * 2 ^ 23 + fraction)
end

local function channel_hex(rgb, shift)
    local byte = math.floor(rgb / 2 ^ shift) % 256
    return float32_hex(byte / 255)
end

local function terminal_colors()
    local colors = {}
    for index = 0, 15 do
        local value = vim.g["terminal_color_" .. index]
        local color
        if type(value) == "string" then
            color = vim.api.nvim_get_color_by_name(value)
        elseif type(value) == "number" then
            color = value
        end
        if type(color) == "number" and color >= 0 and color <= 0xffffff then
            colors[index] = math.floor(color)
        end
    end
    return colors
end

local function terminal_values()
    local values = {}
    for index = 0, 15 do
        local value = vim.g["terminal_color_" .. index]
        if value ~= nil then
            values[index] = vim.deepcopy(value)
        end
    end
    return values
end

local function snapshot_highlights(definitions)
    if not vim.api.nvim_get_hl then
        error("theme_exporter requires Neovim 0.9 or newer")
    end

    definitions = definitions or vim.api.nvim_get_hl(0, {})
    local names = vim.tbl_keys(definitions)
    table.sort(names)

    local highlights = {}
    for _, name in ipairs(names) do
        local ok, highlight = pcall(vim.api.nvim_get_hl, 0, {
            name = name,
            link = false,
        })
        if ok then
            highlights[name] = highlight
        end
    end
    return names, highlights
end

local function snapshot_theme_state()
    if not vim.api.nvim_get_hl then
        error("theme_exporter requires Neovim 0.9 or newer")
    end

    local definitions = vim.api.nvim_get_hl(0, {})
    local names, highlights = snapshot_highlights(definitions)
    return {
        colors_name = vim.g.colors_name,
        definitions = definitions,
        highlights = highlights,
        names = names,
        terminal = terminal_colors(),
        terminal_values = terminal_values(),
    }
end

local function clear_terminal_colors()
    for index = 0, 15 do
        vim.g["terminal_color_" .. index] = nil
    end
end

local function restore_theme_state(state)
    local current = vim.api.nvim_get_hl(0, {})
    for name in pairs(current) do
        vim.api.nvim_set_hl(0, name, {})
    end

    for _, name in ipairs(state.names) do
        local definition = vim.deepcopy(state.definitions[name])
        -- nvim_set_hl() otherwise derives cterm attributes from GUI
        -- attributes, which can make the restored raw definition differ.
        if definition.link == nil and definition.cterm == nil then
            definition.cterm = {}
        end
        vim.api.nvim_set_hl(0, name, definition)
    end

    clear_terminal_colors()
    for index, value in pairs(state.terminal_values) do
        vim.g["terminal_color_" .. index] = value
    end
    vim.g.colors_name = state.colors_name
end

local function colorscheme_is_discoverable(theme_name)
    for _, available_theme in ipairs(vim.fn.getcompletion("", "color")) do
        if available_theme == theme_name then
            return true
        end
    end
    return false
end

local function snapshot_pristine_state()
    clear_terminal_colors()
    vim.cmd("highlight clear")
    local _, highlights = snapshot_highlights()
    return {
        highlights = highlights,
        terminal = terminal_colors(),
    }
end

local function isolate_active_theme()
    if type(vim.g.colors_name) ~= "string" or vim.g.colors_name == "" then
        error("no active named colorscheme to export")
    end

    local active_theme = vim.g.colors_name
    local active = snapshot_theme_state()
    local discoverable = colorscheme_is_discoverable(active_theme)

    if not discoverable then
        local ok, pristine_or_error = pcall(snapshot_pristine_state)
        local restored, restore_error = pcall(restore_theme_state, active)
        if not restored then
            error("failed to restore active colorscheme: " .. tostring(restore_error))
        end
        if not ok then
            error(pristine_or_error, 0)
        end
        return active_theme, pristine_or_error, active
    end

    local pristine = snapshot_pristine_state()

    -- User and plugin ColorScheme autocmds can introduce colors that are not
    -- defined by the theme. The colorscheme file itself remains authoritative.
    vim.cmd("noautocmd colorscheme " .. vim.fn.fnameescape(active_theme))
    return active_theme, pristine
end

local function extract_theme_colors()
    local active_theme, pristine, active = isolate_active_theme()
    local names, highlights
    local current_terminal
    if active then
        names = active.names
        highlights = active.highlights
        current_terminal = active.terminal
    else
        names, highlights = snapshot_highlights()
        current_terminal = terminal_colors()
    end
    local changed = {}
    local color_seen = {}
    local color_values = {}

    local function add_color(color)
        if type(color) ~= "number" or color < 0 or color > 0xffffff then
            return
        end
        color = math.floor(color)
        if not color_seen[color] then
            color_seen[color] = true
            table.insert(color_values, color)
        end
    end

    for _, name in ipairs(names) do
        local highlight = highlights[name]
        local old_highlight = pristine.highlights[name] or {}
        for _, attribute in ipairs(ATTRIBUTES) do
            local color = highlight[attribute]
            if color ~= nil and color ~= old_highlight[attribute] then
                changed[name] = changed[name] or {}
                changed[name][attribute] = true
                add_color(color)
            end
        end
    end

    for index = 0, 15 do
        if current_terminal[index] ~= pristine.terminal[index] then
            add_color(current_terminal[index])
        end
    end

    table.sort(color_values)
    if #color_values == 0 then
        error("the active colorscheme did not define any colors")
    end
    if #color_values > MAX_COLORS then
        error(string.format(
            "theme has %d colors; the palette format supports at most %d",
            #color_values,
            MAX_COLORS
        ))
    end

    return active_theme, color_values, highlights, changed
end

local function rgb_components(color)
    return math.floor(color / 0x10000) % 0x100 / 255,
        math.floor(color / 0x100) % 0x100 / 255,
        color % 0x100 / 255
end

local function linear_channel(channel)
    if channel <= 0.04045 then
        return channel / 12.92
    end
    return ((channel + 0.055) / 1.055) ^ 2.4
end

local function luminance(color)
    local red, green, blue = rgb_components(color)
    return 0.2126 * linear_channel(red)
        + 0.7152 * linear_channel(green)
        + 0.0722 * linear_channel(blue)
end

local function contrast_ratio(first, second)
    local first_luminance = luminance(first)
    local second_luminance = luminance(second)
    local lighter = math.max(first_luminance, second_luminance)
    local darker = math.min(first_luminance, second_luminance)
    return (lighter + 0.05) / (darker + 0.05)
end

local function hsv(color)
    local red, green, blue = rgb_components(color)
    local maximum = math.max(red, green, blue)
    local minimum = math.min(red, green, blue)
    local delta = maximum - minimum
    local hue = 0

    if delta ~= 0 then
        if maximum == red then
            hue = ((green - blue) / delta) % 6
        elseif maximum == green then
            hue = (blue - red) / delta + 2
        else
            hue = (red - green) / delta + 4
        end
        hue = hue / 6
    end

    local saturation = maximum == 0 and 0 or delta / maximum
    return hue, saturation, maximum
end

local function hue_similarity(first, second)
    local distance = math.abs(first - second)
    distance = math.min(distance, 1 - distance)
    return 1 - distance * 2
end

local function color_distance(first, second)
    local first_red, first_green, first_blue = rgb_components(first)
    local second_red, second_green, second_blue = rgb_components(second)
    local red = first_red - second_red
    local green = first_green - second_green
    local blue = first_blue - second_blue
    return math.sqrt(red * red + green * green + blue * blue)
end

local function is_compatible(color, profile, background_color, dark_theme)
    if profile.kind == "background" then
        return color == background_color
    end
    if profile.kind == "surface" or profile.kind == "shadow" then
        return color ~= background_color
            and contrast_ratio(color, background_color) >= 1.05
    end

    local color_luminance = luminance(color)
    local background_luminance = luminance(background_color)
    if profile.hue then
        local color_hue, saturation = hsv(color)
        if saturation < 0.12 or hue_similarity(color_hue, profile.hue) < 0.7 then
            return false
        end
    end
    local correct_polarity = dark_theme and color_luminance > background_luminance
        or not dark_theme and color_luminance < background_luminance
    return correct_polarity
        and contrast_ratio(color, background_color) >= (profile.min_contrast or 3.0)
end

local function semantic_score(color, profile, background_color, dark_theme)
    local ratio = contrast_ratio(color, background_color)
    local color_hue, saturation, value = hsv(color)
    local score

    if profile.kind == "background" then
        score = color == background_color and 1000 or -1000
    elseif profile.kind == "surface" then
        local target = profile.target_contrast or 1.4
        score = 40 - math.abs(ratio - target) * 20 + saturation * 2
    elseif profile.kind == "shadow" then
        score = (1 - luminance(color)) * 30 - math.abs(ratio - 1.4) * 5
    elseif profile.kind == "muted" then
        score = 50 - math.abs(ratio - 3.0) * 8 - saturation * 3
    elseif profile.kind == "foreground" then
        score = ratio * 12
        if profile.neutral then
            score = score - saturation * 8
        end
    elseif profile.kind == "strong" then
        score = ratio * 14 + saturation * 4
    elseif profile.kind == "bright" then
        local polarity_brightness = dark_theme and luminance(color)
            or 1 - luminance(color)
        score = ratio * 15 + polarity_brightness * 12 + saturation * 4
    else
        score = ratio * 12 + saturation * 10 + value * 2
    end

    if profile.hue then
        score = score + hue_similarity(color_hue, profile.hue) * 24
            + saturation * 8
    end
    return score
end

local function determine_background(color_values, highlights, changed)
    local normal = highlights.Normal or {}
    if changed.Normal and changed.Normal.bg and normal.bg then
        return normal.bg
    end

    local best_color = color_values[1]
    local dark_theme = vim.o.background == "dark"
    for _, color in ipairs(color_values) do
        if dark_theme and luminance(color) < luminance(best_color)
            or not dark_theme and luminance(color) > luminance(best_color)
        then
            best_color = color
        end
    end
    return best_color
end

local function source_candidates(profile, highlights, changed)
    local candidates = {}

    local function add(color, weight)
        if type(color) ~= "number" then
            return
        end
        candidates[color] = math.max(candidates[color] or -math.huge, weight)
    end

    for _, source in ipairs(profile.sources or {}) do
        local group_name, attribute, weight = source[1], source[2], source[3]
        local highlight = highlights[group_name]
        if highlight and changed[group_name] and changed[group_name][attribute] then
            add(highlight[attribute], weight)
        end
    end

    if profile.include_bold_groups then
        for group_name, attributes in pairs(changed) do
            local highlight = highlights[group_name]
            if highlight and highlight.bold and attributes.fg then
                add(highlight.fg, 70)
            end
        end
    end
    return candidates
end

local function ranked_colors(
    profile,
    color_values,
    highlights,
    changed,
    background_color,
    dark_theme
)
    if profile.kind == "background" then
        return { background_color }
    end

    local direct = source_candidates(profile, highlights, changed)
    local ranked = {}
    for color, weight in pairs(direct) do
        if is_compatible(color, profile, background_color, dark_theme) then
            table.insert(ranked, {
                color = color,
                score = weight
                    + semantic_score(color, profile, background_color, dark_theme),
            })
        end
    end

    local using_fallback = #ranked == 0
    if using_fallback then
        for _, color in ipairs(color_values) do
            if is_compatible(color, profile, background_color, dark_theme) then
                table.insert(ranked, {
                    color = color,
                    score = semantic_score(
                        color,
                        profile,
                        background_color,
                        dark_theme
                    ),
                })
            end
        end
    end

    -- A pathological one-color palette may have no polarity-compatible
    -- foreground. Reuse the color with the greatest contrast as a last resort
    -- so the JSON contract still contains every documented hint.
    if #ranked == 0 then
        for _, color in ipairs(color_values) do
            table.insert(ranked, {
                color = color,
                score = contrast_ratio(color, background_color),
            })
        end
    end

    table.sort(ranked, function(first, second)
        if first.score == second.score then
            return first.color < second.color
        end
        return first.score > second.score
    end)

    local selected = {}
    local maximum = profile.max_colors or MAX_HINT_COLORS
    if using_fallback then
        maximum = 1
    end
    for _, candidate in ipairs(ranked) do
        local distinct = true
        for _, color in ipairs(selected) do
            if color_distance(color, candidate.color) < 0.12 then
                distinct = false
                break
            end
        end
        if distinct then
            table.insert(selected, candidate.color)
            if #selected == maximum then
                break
            end
        end
    end

    if #selected == 0 then
        table.insert(selected, ranked[1].color)
    end
    return selected
end

local function correct_polarity(color, reference, dark_theme)
    if dark_theme then
        return luminance(color) > luminance(reference)
    end
    return luminance(color) < luminance(reference)
end

local function pole_blend_target(color, dark_theme, amount)
    local red, green, blue = rgb_components(color)
    local pole = dark_theme and 1 or 0
    return red + (pole - red) * amount,
        green + (pole - green) * amount,
        blue + (pole - blue) * amount
end

local function distance_from_target(color, target_red, target_green, target_blue)
    local red, green, blue = rgb_components(color)
    red = red - target_red
    green = green - target_green
    blue = blue - target_blue
    return math.sqrt(red * red + green * green + blue * blue)
end

local function postprocess_background_highlight(
    hints,
    color_values,
    background_color,
    dark_theme
)
    local target_red, target_green, target_blue =
        pole_blend_target(background_color, dark_theme, 0.30)
    local qualifying = {}
    local polarity_matches = {}

    for _, color in ipairs(color_values) do
        if color ~= background_color
            and correct_polarity(color, background_color, dark_theme)
        then
            local candidate = {
                color = color,
                distance = distance_from_target(
                    color,
                    target_red,
                    target_green,
                    target_blue
                ),
                contrast = contrast_ratio(color, background_color),
            }
            table.insert(polarity_matches, candidate)
            if candidate.contrast >= 2.5 then
                table.insert(qualifying, candidate)
            end
        end
    end

    local candidates = #qualifying > 0 and qualifying or polarity_matches
    if #candidates == 0 then
        return
    end
    table.sort(candidates, function(first, second)
        if #qualifying == 0 and first.contrast ~= second.contrast then
            return first.contrast > second.contrast
        end
        if first.distance == second.distance then
            return first.color < second.color
        end
        return first.distance < second.distance
    end)

    local selected = { candidates[1].color }
    for _, color in ipairs(hints["background highlight"]) do
        if color ~= selected[1] and color ~= background_color then
            table.insert(selected, color)
            if #selected == MAX_HINT_COLORS then
                break
            end
        end
    end
    hints["background highlight"] = selected
end

local function postprocess_bold(
    hints,
    color_values,
    background_color,
    dark_theme
)
    local normal_color = hints.normal[1]
    local selected

    local function is_bold_candidate(color)
        return color ~= normal_color
            and color_distance(color, normal_color) >= 0.12
            and correct_polarity(color, background_color, dark_theme)
            and contrast_ratio(color, background_color) >= 3.0
    end

    for _, color in ipairs(hints.bold) do
        if is_bold_candidate(color) then
            selected = color
            break
        end
    end

    if not selected then
        local candidates = {}
        for _, color in ipairs(color_values) do
            if is_bold_candidate(color) then
                table.insert(candidates, {
                    color = color,
                    score = semantic_score(
                        color,
                        HINT_PROFILES.bold,
                        background_color,
                        dark_theme
                    ) + color_distance(color, normal_color) * 20,
                })
            end
        end
        table.sort(candidates, function(first, second)
            if first.score == second.score then
                return first.color < second.color
            end
            return first.score > second.score
        end)
        if #candidates > 0 then
            selected = candidates[1].color
        end
    end

    -- A monochrome palette may not contain a usable alternative.
    if not selected then
        return
    end

    local bold_colors = { selected }
    for _, color in ipairs(hints.bold) do
        if color ~= selected
            and color ~= normal_color
            and color_distance(color, selected) >= 0.12
        then
            table.insert(bold_colors, color)
            if #bold_colors == MAX_HINT_COLORS then
                break
            end
        end
    end
    hints.bold = bold_colors
end

local function append_unique_color(colors, seen, color)
    if type(color) == "number" and not seen[color] then
        seen[color] = true
        table.insert(colors, color)
    end
end

local function selection_backgrounds(
    hints,
    color_values,
    highlights,
    changed,
    background_color
)
    local backgrounds = {}
    local seen = {}
    local visual = highlights.Visual or {}
    if changed.Visual and changed.Visual.bg then
        append_unique_color(backgrounds, seen, visual.bg)
    end
    for _, color in ipairs(hints.selection) do
        append_unique_color(backgrounds, seen, color)
    end

    local remaining = {}
    for _, color in ipairs(color_values) do
        if color ~= background_color and not seen[color] then
            table.insert(remaining, color)
        end
    end
    table.sort(remaining, function(first, second)
        local first_delta = math.abs(
            contrast_ratio(first, background_color) - 1.8
        )
        local second_delta = math.abs(
            contrast_ratio(second, background_color) - 1.8
        )
        if first_delta == second_delta then
            return first < second
        end
        return first_delta < second_delta
    end)
    for _, color in ipairs(remaining) do
        append_unique_color(backgrounds, seen, color)
    end
    if #backgrounds == 0 then
        append_unique_color(backgrounds, seen, background_color)
    end
    return backgrounds
end

local function selection_foregrounds(
    hints,
    color_values,
    highlights,
    changed,
    selection_background,
    dark_theme
)
    local foregrounds = {}
    local seen = {}
    local visual = highlights.Visual or {}
    if changed.Visual and changed.Visual.fg then
        append_unique_color(foregrounds, seen, visual.fg)
    end
    append_unique_color(foregrounds, seen, hints.normal[1])

    local remaining = {}
    for _, color in ipairs(color_values) do
        if color ~= selection_background and not seen[color] then
            table.insert(remaining, color)
        end
    end
    table.sort(remaining, function(first, second)
        local first_contrast = contrast_ratio(first, selection_background)
        local second_contrast = contrast_ratio(second, selection_background)
        if first_contrast == second_contrast then
            return first < second
        end
        return first_contrast > second_contrast
    end)
    for _, color in ipairs(remaining) do
        if correct_polarity(color, selection_background, dark_theme) then
            append_unique_color(foregrounds, seen, color)
        end
    end
    return foregrounds
end

local function postprocess_selection(
    hints,
    color_values,
    highlights,
    changed,
    background_color,
    dark_theme
)
    local backgrounds = selection_backgrounds(
        hints,
        color_values,
        highlights,
        changed,
        background_color
    )
    local best_fallback

    for _, selection_background in ipairs(backgrounds) do
        local foregrounds = selection_foregrounds(
            hints,
            color_values,
            highlights,
            changed,
            selection_background,
            dark_theme
        )
        for _, selection_foreground in ipairs(foregrounds) do
            if selection_foreground ~= selection_background
                and correct_polarity(
                    selection_foreground,
                    selection_background,
                    dark_theme
                )
            then
                local contrast = contrast_ratio(
                    selection_foreground,
                    selection_background
                )
                if contrast >= 4.5 then
                    hints.selection = {
                        selection_foreground,
                        selection_background,
                    }
                    return
                end
                if not best_fallback or contrast > best_fallback.contrast then
                    best_fallback = {
                        foreground = selection_foreground,
                        background = selection_background,
                        contrast = contrast,
                    }
                end
            end
        end
    end

    if best_fallback then
        hints.selection = {
            best_fallback.foreground,
            best_fallback.background,
        }
        return
    end
    error("theme does not contain two colors suitable for selection")
end

local function postprocess_relational_hints(
    hints,
    color_values,
    highlights,
    changed,
    background_color,
    dark_theme
)
    postprocess_background_highlight(
        hints,
        color_values,
        background_color,
        dark_theme
    )
    postprocess_bold(
        hints,
        color_values,
        background_color,
        dark_theme
    )
    postprocess_selection(
        hints,
        color_values,
        highlights,
        changed,
        background_color,
        dark_theme
    )
end

local function build_hints(color_values, highlights, changed)
    local background_color = determine_background(color_values, highlights, changed)
    local dark_theme = luminance(background_color) < 0.5
    local hints = {}

    for _, hint in ipairs(HINT_ORDER) do
        hints[hint] = ranked_colors(
            HINT_PROFILES[hint],
            color_values,
            highlights,
            changed,
            background_color,
            dark_theme
        )
    end
    postprocess_relational_hints(
        hints,
        color_values,
        highlights,
        changed,
        background_color,
        dark_theme
    )
    return hints, dark_theme
end

local function append_color(lines, color, index, count)
    local comma = index < count and "," or ""
    local color_name = json_string(string.format("rgb_%06x", color))
    table.insert(lines, "        {")
    table.insert(lines, "          \"name\": " .. color_name .. ",")
    table.insert(lines, "          \"red\": " .. json_string(channel_hex(color, 16)) .. ",")
    table.insert(lines, "          \"green\": " .. json_string(channel_hex(color, 8)) .. ",")
    table.insert(lines, "          \"blue\": " .. json_string(channel_hex(color, 0)) .. ",")
    table.insert(lines, "          \"alpha\": \"3f800000\"")
    table.insert(lines, "        }" .. comma)
end

local function encode_document(title, color_values, hints)
    local lines = {
        "{",
        "  \"palettes\": [",
        "    {",
        "      \"title\": " .. json_string(title) .. ",",
        "      \"source\": {",
        "        \"conversion_tool\": \"palettetool theme_exporter.nvim\",",
        "        \"conversion_date\": " .. json_string(tostring(os.time())),
        "      },",
        "      \"color_space\": {",
        "        \"name\": \"sRGB\",",
        "        \"is_linear\": false",
        "      },",
        "      \"colors\": [",
    }

    for index, color in ipairs(color_values) do
        append_color(lines, color, index, #color_values)
    end

    table.insert(lines, "      ],")
    table.insert(lines, "      \"hints\": {")
    for index, hint in ipairs(HINT_ORDER) do
        local color_names = {}
        for _, color in ipairs(hints[hint]) do
            table.insert(color_names, string.format("rgb_%06x", color))
        end
        local comma = index < #HINT_ORDER and "," or ""
        local encoded_hint = json_string(color_names)
        table.insert(lines, "        " .. json_string(hint) .. ": " .. encoded_hint .. comma)
    end

    table.insert(lines, "      },")
    table.insert(lines, "      \"gradients\": {},")
    table.insert(lines, "      \"dither_pairs\": {}")
    table.insert(lines, "    }")
    table.insert(lines, "  ]")
    table.insert(lines, "}")
    return lines
end

---Export the isolated active colorscheme to a palettetool JSON palette.
---@param path string
---@return table result Contains path, color_count, hint_count, and polarity.
function M.export(path)
    if type(path) ~= "string" then
        error("path must be a string")
    end
    if path == "" then
        error("path must not be empty")
    end

    local title, color_values, highlights, changed = extract_theme_colors()
    local hints, dark_theme = build_hints(color_values, highlights, changed)
    title = truncate_string(title)
    local lines = encode_document(title, color_values, hints)
    local expanded_path = vim.fn.expand(path)
    local write_result = vim.fn.writefile(lines, expanded_path)
    if write_result ~= 0 then
        error("failed to write palette to " .. expanded_path)
    end

    local result = {
        path = expanded_path,
        color_count = #color_values,
        hint_count = #HINT_ORDER,
        polarity = dark_theme and "dark" or "light",
    }
    vim.notify(string.format(
        "Exported %d %s-theme colors from %s to %s",
        result.color_count,
        result.polarity,
        title,
        expanded_path
    ))
    return result
end

local function filename_for_theme(theme_name)
    local filename = theme_name:gsub("%s+", "-")
    filename = filename:gsub("[/\\]", "-")
    if filename == "" then
        error("active colorscheme has no usable filename")
    end
    return filename .. ".pal.json"
end

---Load an optional colorscheme and export it into a directory.
---@param directory string
---@param requested_theme? string
---@return table result Also contains the generated filename.
function M.export_theme(directory, requested_theme)
    if type(directory) ~= "string" or directory == "" then
        error("directory must be a non-empty string")
    end
    if requested_theme ~= nil and type(requested_theme) ~= "string" then
        error("requested_theme must be a string or nil")
    end
    if requested_theme and requested_theme ~= ""
        and not (
            requested_theme == vim.g.colors_name
            and not colorscheme_is_discoverable(requested_theme)
        )
    then
        vim.cmd.colorscheme(requested_theme)
    end
    if type(vim.g.colors_name) ~= "string" or vim.g.colors_name == "" then
        error("no active named colorscheme to export")
    end

    local filename = filename_for_theme(vim.g.colors_name)
    local expanded_directory = vim.fn.expand(directory)
    local separator = expanded_directory:sub(-1) == "/" and "" or "/"
    local result = M.export(expanded_directory .. separator .. filename)
    result.filename = filename
    return result
end

return M
