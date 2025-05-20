# Limitations #

## Colorspaces ##

The current version of palettetool is colorspace-naive.  This makes it bad for serious usage.  However, in practice, it's usually fine for simplistic uses.

Some upgrades need to be made in order for it to be a *correct* palette interchange format.  From there, further upgrades need to be made for it to be a *convenient* palette interchange format.

### The Colorspace Plan ###

The palette document will be expanded to include:
 
 - A colorspace name, eg: "srgb", "linear-srgb"
 - A filename of an ICC file that corresponds to the colorspace
 - A boolean flag indicating whether the colorspace is linear
 
The recognized set of colorspace names are:

| Name                              | Gamut           | White Point | Transfer Function          |
| --------------------------------- | --------------- | ----------- | -------------------------- |
| `"sRGB"` or `"sRGB IEC61966-2.1"` | Standard sRGB   | D65         | sRGB TRC                   |
| `"linear-sRGB"`                   | sRGB gamut      | D65         | linear                     |
| `"Display P3"`                    | Wide            | D65         | sRGB TRC                   |
| `"linear-Display P3"`             | Wide            | D65         | linear                     |
| `"AdobeRGB1998"`                  | Medium-wide     | D65         | gamma 2.2                  |
| `"linear-AdobeRGB1998"`           | Medium-wide     | D65         | linear                     |
| `"Rec.709"`                       | Similar to sRGB | D65         | gamma 2.4                  |
| `"ACEScg"`                        | Wide            | D60         | linear                     |
| `"Rec.2020"`                      | Very wide       | D65         | gamma 2.4 or PQ            |
| `"scRGB"`                         | sRGB gamut      | D65         | linear (can go <0.0, >1.0) |

There are a few things to consider here:

 1. for the foreseeable future, ICC parsing and color translation is out of scope for this project.  I just don't have the time to donate.
 
 2. The colorespace name and linear flag are not normalized, and one or both can contradict the ICC profile.  The ICC profile should be considered authoritative when this happens.
 
Users who care about ICC color profile matching should use an external library to do the job.  Internally, there will be functions to convert palette data into linear sRGB, interpreting it as standard sRGB on input.

For each conversion format, palettetool will handle it accordingly:

 1. **PNG** data is assumed to be sRGB encoded.  The gAMA, sRGB and iCCP chunks are ignored, as they are often wrong.  The image is converted into `linear-sRGB` on input.  `linear-sRGB` is consided the "default" or "destination" format for palettetool, and is the target conversion space unless specified. 
 
 2. **ACO** data is also assumed to be sRGB encoded.  ACO files include no encoding information.  However, their primary use is in reading ACO files exported by Photopea, which is web-based and is therefore likely to be sRGB based.
 
 3. **JSON** palettetool images indicate what colorspace the image is in.  If it is in `linear-sRGB`, the output images will be converted into sRGB unless otherwise specified.

## HDR ##

High dynamic range is relatively open-ended.  You specify floats greater than 1.0, and that means something for your colorspace.

If a palette is being converted into sRGB and it has HDR (channels with < 1.0) representation, the result will be clamped.  Tone mapping may be possible in the future.  If this is disagreeable, avoid the conversion.
