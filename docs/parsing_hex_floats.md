
# Floating Point Hex #

The Open Palette format stores each channel as a 32-bit float encoded as a hex value inside of a string in the JSON.  Converting to decimal strings forces a lossy "translation" of a binary repeating fraction into a base-10 approximation, which leads to bit-level drift and rounding discrepancies when different languages or hardware architectures try to parse that approximation back into binary.

First time viewers may assume the colors are hex color values, as are common in CSS like `#FF00A0FF`. That would not be accurate. They are intended to encode a value between 0 and 1, or higher than 1 in the case of out of gamut intensity.

What follows are some routines that prove useful to perform correct conversions:

## C ##


```c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void float32_to_hex(float f, char* out_hex) {
    uint32_t bits;
    memcpy(&bits, &f, sizeof(float));
    sprintf(out_hex, "%08x", bits);
}

float hex_to_float32(const char* hex) {
    uint32_t bits = (uint32_t)strtoul(hex, NULL, 16);
    float f;
    memcpy(&f, &bits, sizeof(float));
    return f;
}
```

## Python ##

```python
import struct

def f32_to_hex(f: float) -> str:
    return struct.pack('>f', f).hex()

def hex_to_f32(h: str) -> float:
    return struct.unpack('>f', bytes.fromhex(h))[0]

# Example:
hex_val = float32_to_hex(0.5)  # "3f000000"
float_val = hex_to_float32(hex_val) # 0.5
```

## JavaScript ##

```javascript
function f32ToHex(f) {
    const f32 = new Float32Array([f]);
    const u32 = new Uint32Array(f32.buffer);
    return u32[0].toString(16).padStart(8, '0');
}

function hexToF32(hexStr) {
    const u32 = new Uint32Array([parseInt(hexStr, 16)]);
    const f32 = new Float32Array(u32.buffer);
    return f32[0]; 
}

// Example:
const hexVal = f3232ToHex(0.5); // "3f000000"
const floatVal = hexToF32(hexVal); // 0.5
```
