#!/usr/bin/env python

# Given a palette .json and an arbitrary jinja template, generate
# something that is useful for the user

import json
import argparse

import jinja2

def parse_args():
    p = argparse.ArgumentParser(
        prog="palette2jinja",
        description="Given a palette .json and an arbitrary jinja template, generate something that is useful for the user")
    p.add_argument('json_filename')
    p.add_argument('tmpl_filename')
    p.add_argument('-o', '--output')
    return p.parse_args()


def channel_to_8bit(val):
    val = max(0.0, min(1.0, val))

    fixed = int(val * 256.0)
    reduced = 127 if fixed == 128 else (fixed * 255 + 192) >> 8

    return reduced

def parse_palette(path):
    f = open(path)
    data = json.load(f)
    f.close()
    # do absolutely no schema verification
    return data

# probably should do one for rgba
def filter_hexcolor_rgb(color):
    return str(hex(channel_to_8bit(color['red'])))[2:] + \
        str(hex(channel_to_8bit(color['green'])))[2:] + \
        str(hex(channel_to_8bit(color['blue'])))[2:]

def filter_to_8bit(val):
    return channel_to_8bit(val)


if __name__ == '__main__':
    args = parse_args()

    palette_doc = parse_palette(args.json_filename)

    with open(args.tmpl_filename) as f:
        tmpl_bytes = f.read()

    env = jinja2.Environment()
    env.filters['hexcolor_rgb'] = filter_hexcolor_rgb
    env.filters['to_8bit'] = filter_to_8bit
    tmpl = env.from_string(tmpl_bytes)

    rendered = tmpl.render(doc=palette_doc)

    if args.output:
        with open(args.output, 'w', encoding='utf-8') as f:
            f.write(rendered)
    else:
        print(rendered)
