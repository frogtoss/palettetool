#!/usr/bin/env python

import os
import sys
import subprocess

if 'clean' in sys.argv:
    os.unlink('palettetool')
    sys.exit(1)

if not os.path.isdir('bin'):
    os.mkdir("bin")

build_cmd = "clang src/palettetool.c -Wall -fno-strict-aliasing -O3 -o bin/palettetool"
cp = subprocess.run(build_cmd, shell=True)
sys.exit(cp.returncode)
