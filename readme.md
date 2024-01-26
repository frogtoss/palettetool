# Palette Tool #

Convert between palette formats.

Includes support for an 'open palette' JSON format, which has the following features:

 - easy to read and parse
 - source fields exist to credit original author
 - 32-bits per channel
 - named colours
 - usage hints for colours to allow for colour theme conversions between programs
 - specify relationships between colours with gradients and dithers
 - MIT C source reader/writer support for format exists


## Usage ##

    # convert from adobe aco (v2) to palette json format
    palettetool --in swatch.aco --out swatch_palette.json
    
    # convert from adobe aco (v2) to png
    palettetool --in swatch.aco --out image.png
    
    # see the full list of supported input/output formats
    palettetool --help
    
    # convert from palette json format to png
    palettetool --in swatch_palette.json --out image.png

    # as above, but sort the colors based on brightness
    palettetool --in swatch_palette.json --out image.png --sort-png brightness

## Build ##

No submodules, libs or dependencies other than libc.

Building produces standalone palettetool executable in `\bin`

### From Commandline ###

If you're on Windows, make sure the visual c++ environment is in your program.

Regardless of OS, then run:

    make.py --skip-premake
    
Tested on Windows and Linux.

### From Visual Studio ###

Open `build\vs2022\Palette Tool.sln` in explorer and build.
