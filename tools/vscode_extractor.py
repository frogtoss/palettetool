#!/usr/bin/env python

# vscode theme extractor
# usage: launch the theme in
# vscode and choose 'Developer: Generate Color Theme From Current Settings'
#
# save that file and run this script with that file as first param:
# vscode_extractor.py your_file.json > out.pal.json


import sys
import json
import struct
import time
import os
import zlib

def f32_to_hex(f: float) -> str:
    """Pack a float32 into a big-endian hex string."""
    return struct.pack('>f', f).hex()

def clean_jsonc(text: str) -> str:
    """
    Safely removes // line comments, /* block comments */, and trailing commas
    from a JSON string without destroying URLs or string contents.
    """
    out = []
    in_string = False
    escape = False
    i = 0
    n = len(text)
    
    # Pass 1: Remove Comments
    while i < n:
        c = text[i]
        if not in_string:
            if c == '/' and i + 1 < n and text[i+1] == '/':
                # Skip to end of line
                while i < n and text[i] != '\n':
                    i += 1
                continue
            elif c == '/' and i + 1 < n and text[i+1] == '*':
                # Skip to end of block comment
                i += 2
                while i + 1 < n and not (text[i] == '*' and text[i+1] == '/'):
                    i += 1
                i += 2
                continue
            elif c == '"':
                in_string = True
        else:
            if c == '\\' and not escape:
                escape = True
            else:
                if c == '"' and not escape:
                    in_string = False
                escape = False
                
        out.append(c)
        i += 1

    # Pass 2: Remove Trailing Commas
    cleaned_str = "".join(out)
    out2 = []
    in_string = False
    escape = False
    
    i = 0
    n = len(cleaned_str)
    while i < n:
        c = cleaned_str[i]
        if not in_string:
            if c == '"':
                in_string = True
                out2.append(c)
            elif c in ' \n\r\t':
                out2.append(c)
            elif c in '}]':
                # Walk back and erase trailing comma if it exists
                j = len(out2) - 1
                while j >= 0 and out2[j] in ' \n\r\t':
                    j -= 1
                if j >= 0 and out2[j] == ',':
                    out2[j] = ' ' # Erase the trailing comma safely
                out2.append(c)
            else:
                out2.append(c)
        else:
            if c == '\\' and not escape:
                escape = True
            else:
                if c == '"' and not escape:
                    in_string = False
                escape = False
            out2.append(c)
        i += 1
        
    return "".join(out2)

def parse_vscode_hex(hex_str: str) -> tuple:
    """Converts a VS Code hex string to a tuple of f32 hex strings."""
    hex_str = hex_str.lstrip('#')
    
    if len(hex_str) == 6:
        r, g, b, a = int(hex_str[0:2], 16), int(hex_str[2:4], 16), int(hex_str[4:6], 16), 255
    elif len(hex_str) == 8:
        r, g, b, a = int(hex_str[0:2], 16), int(hex_str[2:4], 16), int(hex_str[4:6], 16), int(hex_str[6:8], 16)
    else:
        r, g, b, a = 0, 0, 0, 255

    return (
        f32_to_hex(r / 255.0),
        f32_to_hex(g / 255.0),
        f32_to_hex(b / 255.0),
        f32_to_hex(a / 255.0)
    )

# Strictly enforced allowed hints
ALLOWED_HINTS = [
    "error", "warning", "normal", "success", "highlight", "urgent", 
    "low priority", "bold", "background", "background highlight", 
    "focal point", "title", "subtitle", "subsubtitle", "todo", "fixme", 
    "sidebar", "subtle", "shadow", "specular", "selection", "comment", 
    "string", "keyword", "variable", "operator", "punctuation", 
    "inactive", "function", "method", "preprocessor", "type", 
    "constant", "link", "cursor"
]

# Map VS Code UI elements to Hint categories
UI_MAPPING = {
    "editor.background": "background",
    "editor.foreground": "normal",
    "editorCursor.foreground": "cursor",
    "editor.selectionBackground": "selection",
    "editor.selectionForeground": "selection",
    "editor.lineHighlightBackground": "background highlight",
    "editorError.foreground": "error",
    "editorWarning.foreground": "warning",
    "editorInfo.foreground": "highlight",
    "sideBar.background": "sidebar",
    "sideBar.foreground": "sidebar",
    "widget.shadow": "shadow",
    "focusBorder": "focal point",
    "tab.inactiveForeground": "inactive",
    "editorLineNumber.foreground": "subtle",
    "editorLink.activeForeground": "link",
    "diffEditor.insertedTextBackground": "success",
    "diffEditor.removedTextBackground": "error",
    "editor.findMatchBackground": "highlight"
}

# Map TextMate scopes to Hint categories
TOKEN_MAPPING = {
    "comment": "comment",
    "string": "string",
    "keyword": "keyword",
    "storage": "keyword",
    "variable": "variable",
    "keyword.operator": "operator",
    "punctuation": "punctuation",
    "entity.name.function": "function",
    "entity.name.method": "method",
    "meta.preprocessor": "preprocessor",
    "entity.name.type": "type",
    "support.type": "type",
    "constant": "constant",
    "markup.bold": "bold",
    "markup.heading.1": "title",
    "markup.heading.2": "subtitle",
    "markup.heading.3": "subsubtitle",
    "markup.underline.link": "link",
    "invalid": "error"
}

def extract_theme(input_path: str):
    with open(input_path, 'r', encoding='utf-8') as f:
        raw_text = f.read()

    # Sanitize JSONC -> standard JSON
    clean_text = clean_jsonc(raw_text)
    theme_data = json.loads(clean_text)

    # State for our unique colors and hints
    color_palette = {} # Mapping of f32 tuple -> unique color name
    hints = {hint: [] for hint in ALLOWED_HINTS}
    
    def add_color_to_hint(hex_str: str, hint_name: str):
        if not hex_str:
            return
            
        color_tuple = parse_vscode_hex(hex_str)
        
        # Deduplicate color and assign a name if it's new
        if color_tuple not in color_palette:
            color_name = f"color_{len(color_palette):02x}"
            
            if len(color_palette) >= 256: # Hard limit
                return 
                
            color_palette[color_tuple] = color_name
            
        color_name = color_palette[color_tuple]
        
        # Add to hint array without duplicating it inside that specific hint
        if color_name not in hints[hint_name]:
            hints[hint_name].append(color_name)

    vs_colors = theme_data.get("colors", {})

    # 1. GUARANTEE: Background is processed first so it's index 0 in the 'background' hint
    if "editor.background" in vs_colors:
        add_color_to_hint(vs_colors["editor.background"], "background")

    # 2. Extract remaining UI Colors
    for vs_key, custom_hint in UI_MAPPING.items():
        if vs_key == "editor.background":
            continue # Already handled
        if vs_key in vs_colors:
            add_color_to_hint(vs_colors[vs_key], custom_hint)

    # 3. Extract Token Colors (Syntax Highlighting)
    token_colors = theme_data.get("tokenColors", [])
    for rule in token_colors:
        scopes = rule.get("scope", [])
        if isinstance(scopes, str):
            scopes = [scopes]
            
        settings = rule.get("settings", {})
        fg_val = settings.get("foreground")
        bg_val = settings.get("background")
        
        if not fg_val and not bg_val:
            continue
            
        for scope in scopes:
            for map_scope, custom_hint in TOKEN_MAPPING.items():
                if scope == map_scope or scope.startswith(map_scope + "."):
                    if bg_val:
                        add_color_to_hint(bg_val, custom_hint)
                    if fg_val:
                        add_color_to_hint(fg_val, custom_hint)

    # 4. Format output structures
    output_colors = []
    for color_tuple, name in color_palette.items():
        output_colors.append({
            "name": name,
            "red": color_tuple[0],
            "green": color_tuple[1],
            "blue": color_tuple[2],
            "alpha": color_tuple[3]
        })

    # Drop empty hints to keep the JSON payload clean
    output_hints = {k: v for k, v in hints.items() if len(v) > 0}

    hash_payload = json.dumps(output_colors, sort_keys=True).encode('utf-8')
    color_hash = str(zlib.crc32(hash_payload))
    filename = os.path.basename(input_path).replace('.json', '')

    output_document = {
        "palettes": [
            {
                "title": filename,
                "color_hash": color_hash,
                "source": {
                    "conversion_tool": "ftg_palette vscode_extractor.py",
                    "conversion_date": str(int(time.time()))
                },
                "color_space": {
                    "is_linear": False
                },
                "colors": output_colors,
                "hints": output_hints,
                "gradients": {},
                "dither_pairs": {}
            }
        ]
    }

    return output_document

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python vscode_extractor.py <path_to_vscode_theme.json>")
        sys.exit(1)

    input_file = sys.argv[1]
    
    try:
        result = extract_theme(input_file)
        print(json.dumps(result, indent=4))
    except json.JSONDecodeError as e:
        print(f"JSON Parsing Error after cleanup: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error parsing theme: {e}", file=sys.stderr)
        sys.exit(1)
