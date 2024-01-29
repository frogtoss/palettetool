#!/usr/bin/env python

# A hack to extract hex values and possible color names out of a document.
# It scans for hex values and then extracts the first probably keyword from before it.
#
# it outputs open palette documents


import os
import re
import sys
import json
import time
import argparse


def fatal(msg):
    print("fatal: " + str(msg), file=sys.stderr)
    exit(1)

def parse_args():
    p = argparse.ArgumentParser(
        prog="Extractor",
        description="Extract named hex colors from a document")
    p.add_argument('filename')
    return p.parse_args()

def epoch_time_string():
    return str(int(time.time()))

def read_file_into_string(file_path):
    try:
        with open(file_path, "rb") as f:
            return f.read().decode('utf-8')
    except or as e:
        fatal("Error reading file: " + str(e))

def init_palette_doc():
    doc = {
        'palettes': [
            {
                'title': "untitled",
                'source': {
                    'conversion_tool': "ftg_palette extractor.py",
                    'conversion_date': epoch_time_string(),
                },
                'colors': [],
                'hints': {},
                'gradients': {},
                'dither_pairs': {},
            }
        ]
    }
    return doc

def is_valid_hex_color(s):
    re_hex_color = re.compile(r"^#?[a-fA-F0-9]+$")
    if not re_hex_color.match(s):
        return False
    return is_valid_hex_len(s)

def is_valid_hex_len(s):
    l = len(s)
    if s[0] == '#': l -= 1
    return l == 3 or l == 6 or l == 8

def algo_extract_hex_find_previous_keyword(file_contents):
    # color_keyvalues entries should be ['some_key', 'hex_color_value]
    color_keyvalues = []

    re_find_hex = re.compile("(#[0-9a-fA-F]+)")
    for line in file_contents.splitlines():
        matches = re_find_hex.findall(line)
        if len(matches) == 0:
            continue

        line_tokens = [x for x in re.split(r"[^a-zA-Z0-9\-\#]", line) if x]

        # line can be like ['fg-2', '#E0E0E0', 'fg-3', '#E2E9E9']
        if len(line_tokens) % 2 != 0:
            continue

        for i in range(0, len(line_tokens), 2):
            color_keyvalues.append([line_tokens[i], line_tokens[i+1]])


    return color_keyvalues

def channel_to_f32(val):
    if val == 127:
        return 0.5
    if val == 255:
        return 1.0
    return val / 256.0

def color_from_hexcolor(name, hex_str):
    color = {'name': name}
    hex_str = hex_str.lstrip('#')

    # fixme: this only does rgb 6-char
    if len(hex_str) != 6:
        fatal("todo: support hex other than 6-char.  got: " + hex_str)

    rgb = tuple(int(hex_str[i:i+2], 16) for i in (0, 2, 4))
    color['red'] = channel_to_f32(rgb[0])
    color['green'] = channel_to_f32(rgb[1])
    color['blue'] = channel_to_f32(rgb[2])
    color['alpha'] = 1.0

    return color

def add_color_keyvalues_to_doc(doc, color_keyvalues):
    for color_keyvalue in color_keyvalues:
        doc['palettes'][0]['colors'].append(color_from_hexcolor(color_keyvalue[0], color_keyvalue[1]))

if __name__ == '__main__':
    args = parse_args()
    file_contents = read_file_into_string(args.filename)

    doc = init_palette_doc()

    # can add other algos in the future
    color_keyvalues = algo_extract_hex_find_previous_keyword(file_contents)
    add_color_keyvalues_to_doc(doc, color_keyvalues)

    print(json.dumps(doc, indent=4))
