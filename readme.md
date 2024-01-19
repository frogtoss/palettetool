# Palette Tool #

Convert between palette formats.


## Usage ##

    # convert from adobe aco (v2) to palette json format
    palettetool --in swatch.aco --out swatch_palette.json
    
    # convert from adobe aco (v2) to png
    palettetool --in swatch.aco --out image.png

## Build ##

No dependencies other than libc.

Make sure clang is installed, then:

    make.py
    
bin/palette is created.

Tested on Linux.
