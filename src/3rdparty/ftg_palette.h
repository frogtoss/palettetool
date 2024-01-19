/* ftg_palette  - public domain library
   no warranty implied; use at your own risk

   <brief description here>

   USAGE

   Do this:
   #define FTG_IMPLEMENT_PALETTE

   before you include this file in one C or C++ file to create the
   implementation.

   It should look like this:
   #include ...
   #include ...
   #include ...
   #define FTG_IMPLEMENT_PALETTE
   #include "ftg_palette.h"

   REVISION HISTORY

   0.1  (tododate)   Initial version

   LICENSE

   This software is in the public domain. Where that dedication is not
   recognized, you are granted a perpetual, irrevocable license to copy,
   distribute, and modify this file as you see fit.

   Special thanks to Sean T. Barrett (@nothings) for the idea to use single
   header format for libraries.
*/

#ifndef PAL__INCLUDE_PALETTE_H
#define PAL__INCLUDE_PALETTE_H

//// DOCUMENTATION
//
//
//// Basic Usage
//
// <basic usage here>

#ifdef PAL_PALETTE_STATIC
#    define PALDEF static
#else
#    define PALDEF extern
#endif

#ifdef PAL_PALETTE_STATIC
#    define PALDEFDATA static
#else
#    define PALDEFDATA
#endif


#if defined(PAL_MALLOC) && defined(PAL_FREE) && defined(PAL_REALLOC)
// okay
#elif !defined(PAL_MALLOC) && !defined(PAL_FREE) && !defined(PAL_REALLOC)
// also okay
#else
#    error "Must define all or none of PAL_MALLOC, PAL_FREE and PAL_REALLOC."
#endif

#ifndef PAL_MALLOC
#    define PAL_MALLOC(size) malloc(size)
#    define PAL_REALLOC(ptr, size) realloc(ptr, size)
#    define PAL_FREE(ptr) free(ptr)
#endif

// include ftg_core.h ahead of this header to debug it
#ifdef FTG_ASSERT
#    define PAL__ASSERT(exp) FTG_ASSERT(exp)
#    define PAL__ASSERT_FAIL(exp) FTG_ASSERT_FAIL(exp)
#else
#    define PAL__ASSERT(exp) ((void)0)
#    define PAL__ASSERT_FAIL(exp) ((void)0)
#endif



#ifdef __cplusplus
#    extern "C"
#endif

#define PAL_MAX_COLORS 255
#define PAL_MAX_GRADIENT_INDICES (PAL_MAX_COLORS * 2)
#define PAL_MAX_STRLEN 48
#define PAL_MAX_HINTS 4
#define PAL_MAX_GRADIENTS 32
#define PAL_MAX_DITHER_PAIRS (PAL_MAX_COLORS * 2)

typedef char           pal_str_t[PAL_MAX_STRLEN];
typedef unsigned char  pal_u8_t;
typedef unsigned short pal_u16_t;
typedef unsigned int   pal_u32_t;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} pal_rgba_t;

typedef struct {
    union {
        pal_rgba_t rgba;
        float      c[4];
    };

} pal_color_t;

typedef struct {
    pal_str_t          url;
    pal_str_t          conversion_tool;
    unsigned long long conversion_timestamp;
} pal_source_t;

typedef enum {
    HINT_ERROR,
    HINT_WARNING,
    HINT_NORMAL,
    HINT_SUCCESS,
    HINT_HIGHLIGHT,
    HINT_URGENT,
    HINT_LOW_PRIORITY,
    HINT_BOLD,
    HINT_BACKGROUND,
    HINT_BACKGROUND_HIGHLIGHT,
    HINT_FOCAL_POINT,
    HINT_TITLE,
    HINT_SUBTITLE,
    HINT_SUBSUBTITLE,
    HINT_TODO,
    HINT_FIXME,
    HINT_SIDEBAR,
    HINT_SUBTLE,
    HINT_SHADOW,
    HINT_SPECULAR,
} pal_hint_kind_t;

typedef struct {
    int       num_indices;
    pal_u16_t indices[PAL_MAX_GRADIENT_INDICES];
} pal_gradient_t;

typedef struct {
    pal_u16_t index0;
    pal_u16_t index1;
} pal_dither_pair_t;

typedef struct {
    unsigned short version;
    pal_str_t      title;
    pal_source_t   source;

    pal_u16_t   num_colors;
    pal_str_t   color_names[PAL_MAX_COLORS];
    pal_color_t colors[PAL_MAX_COLORS];

    pal_u16_t       num_hints[PAL_MAX_COLORS];
    pal_hint_kind_t hints[PAL_MAX_COLORS][PAL_MAX_HINTS];

    pal_u16_t      num_gradients;
    pal_str_t      gradient_names[PAL_MAX_GRADIENTS];
    pal_gradient_t gradients[PAL_MAX_GRADIENTS];

    pal_u16_t         num_dither_pairs;
    pal_str_t         dither_pair_names[PAL_MAX_DITHER_PAIRS];
    pal_dither_pair_t dither_pairs[PAL_MAX_DITHER_PAIRS];
} pal_palette_t;

// API declaration starts here


// Convert a value in range 0-1 to an 8-bit channel between 0x0 and 0xFF
pal_u8_t pal_convert_channel_to_8bit(float val);

// convert a value in range 0x0 to 0xFF
float pal_convert_channel_to_f32(pal_u8_t val);

// parse an adobe aco file into a pal_palette_t
// currently only works on v2 (as used by Photopea)
//
// if aco_source_url is NULL, no URL will be specified
int pal_parse_aco(const unsigned char* bytes,
                  unsigned int         len,
                  pal_palette_t*       out_pal,
                  const char*          aco_source_url);

// allows a quick 32-bit integer comparison to test for color
// equivalency between palettes
//
// provide a 32-bit value that is a hash of all colors in the palette
// color names, gradients, stipples, etc. do not affect this value
// color order affects the value
pal_u32_t pal_hash_color_values(const pal_palette_t* pal);

// string names for hint enums
const char* pal_string_for_hint(pal_hint_kind_t hint);

// emit a palette json file with num_pals palettes into out_buf
//
// out_buf must be preallocated to store the json document, with
// out_buf_len being the length of that buffer.
int pal_emit_palette_json(const pal_palette_t* pals, int num_pals, char* out_buf, int out_buf_len);

//
// End of header file
//
#endif /* PAL__INCLUDE_PALETTE_H */

/* implemetnation */
#if defined(FTG_IMPLEMENT_PALETTE)

#ifndef PAL_TIME
#    include <time.h>
#    define PAL_TIME(n) time(n)
#endif

static int
pal__append_buf(char** out_buf, int* out_buf_remaining, const char* str)
{
    while (*str) {
        if (*out_buf_remaining == 0) {
            PAL__ASSERT(!"Ran out of space appending buf");
            return 1;
        }

        **out_buf = *str;
        str++;
        (*out_buf)++;
        (*out_buf_remaining)--;
    }

    return 0;
}

#define PAL__TAB "    "
#define PAL__3TAB PAL__TAB PAL__TAB PAL__TAB

static int
pal__append_buf_tabs(char** out_buf, int* out_buf_remaining, int num_tabs)
{
    int i;
    for (i = 0; i < num_tabs; i++) {
        int result = pal__append_buf(out_buf, out_buf_remaining, PAL__TAB);
        if (result != 0)
            return result;
    }

    return 0;
}

static char*
pal__int_to_str(unsigned long long val, char* buf, int len)
{
    // write to the end of the given buffer, not the beginning, and
    // reverse towards greatest digit.
    // return the pointer to the beginning of the string
    char* p_buf = buf + len - 1;
    *p_buf-- = 0;

    do {
        unsigned long long digit = val % 10;
        val /= 10;
        *--p_buf = '0' + (char)digit;
    } while (val && p_buf >= buf);

    return p_buf;
}

static int
pal__float_to_str(float val, char* buf, int len)
{
    char* p_buf = buf;
    // routine only needs to work with non-inf, non NaN values between
    // 0 and 1 while ignoring locale
    PAL__ASSERT(val >= 0.0f && val <= 1.0f);
    if (len < 4)
        return 1;

    const int PRECISION = 8;
    int       i;

    if (val == 1.0f) {
        *p_buf++ = '1';
        *p_buf++ = '.';
        *p_buf++ = '0';
        *p_buf = 0;
        return 0;
    }

    *p_buf++ = '0';
    *p_buf++ = '.';

    for (i = 0; i < PRECISION; i++) {
        if (p_buf - buf - 1 >= len)
            return 1;

        val *= 10.f;
        int digit = (char)val;
        PAL__ASSERT(digit >= 0 || digit <= 9);
        char num = '0' + digit;
        *p_buf++ = num;
        val -= (float)digit;
    }

    // todo: remove training zeroes up to tens for aesthetic reasons
    *p_buf = 0;

    return 0;
}

#define PAL__APPEND(s)                                                         \
    result |= pal__append_buf(&out_buf, &out_buf_remaining, (s))

#define PAL__APPEND_TABS(n)                                                    \
    result |= pal__append_buf_tabs(&out_buf, &out_buf_remaining, (n));

// append a single "foo": "bar" to the buf
#define PAL__APPEND_JSON_KEYVALUE_STRING(key, value, trailing_comma)           \
    pal__append_buf_tabs(&out_buf, &out_buf_remaining, tab);                   \
    PAL__APPEND("\"" key "\": \"");                                            \
    PAL__APPEND((char*)&value[0]);                                             \
    PAL__APPEND(TRAILING_COMMA[trailing_comma]);                               \
    if (result != 0) {                                                         \
        PAL__ASSERT(!"buf overflow");                                          \
        return 1;                                                              \
    }

#define PAL__APPEND_JSON_KEYVALUE_NOQUOTE(key, value, trailing_comma)          \
    pal__append_buf_tabs(&out_buf, &out_buf_remaining, tab);                   \
    PAL__APPEND("\"" key "\": ");                                              \
    PAL__APPEND((char*)&value[0]);                                             \
    PAL__APPEND(TRAILING_COMMA[trailing_comma]);                               \
    if (result != 0) {                                                         \
        PAL__ASSERT(!"buf overflow");                                          \
        return 1;                                                              \
    }

#define PAL__WALK_BACK(n) out_buf -= (n), out_buf_remaining -= (n);



PALDEF int
pal_emit_palette_json(const pal_palette_t* pals, int num_pals, char* out_buf, int out_buf_len)
{
    int         out_buf_remaining = out_buf_len;
    int         tab = 0;
    int         result = 0;
    int         i, j, k;
    const char* TRAILING_COMMA[] = {"\"\n", "\",\n", "\n", ",\n", "", ","};

    PAL__APPEND("{\n" PAL__TAB "\"palettes\": [\n");

    //
    // for each palette
    tab += 2;

    for (i = 0; i < num_pals; i++) {
        const pal_palette_t* pal = &pals[i];
        char                 num_buf[64];
        char*                p_num_buf;

        // todo: UID field that hashes all colors


        // palette sub-document
        PAL__APPEND_TABS(tab++);
        PAL__APPEND("{\n");



        // title
        PAL__APPEND_JSON_KEYVALUE_STRING("title", pal->title, 1);

        // color hash
        p_num_buf = pal__int_to_str(pal_hash_color_values(pal), num_buf, 64);
        PAL__APPEND_JSON_KEYVALUE_STRING("color_hash", p_num_buf, 1);

        //
        // source block
        //
        PAL__APPEND_TABS(tab++);
        PAL__APPEND("\"source\": {\n");
        if (pal->source.url[0]) {
            PAL__APPEND_JSON_KEYVALUE_STRING("url", pal->source.url, 1);
        }
        if (pal->source.conversion_tool[0]) {
            PAL__APPEND_JSON_KEYVALUE_STRING(
                "conversion_tool", pal->source.conversion_tool, 1);
        }

        p_num_buf = pal__int_to_str(pal->source.conversion_timestamp, num_buf, 64);
        PAL__APPEND_JSON_KEYVALUE_STRING("conversion_date", p_num_buf, 0);
        tab--;
        PAL__APPEND(PAL__3TAB "},\n\n");  // source

        //
        // colors block
        //
        PAL__APPEND_TABS(tab++);
        PAL__APPEND("\"colors\": [\n");
        for (j = 0; j < pal->num_colors; j++) {
            result |= pal__append_buf_tabs(&out_buf, &out_buf_remaining, tab++);
            PAL__APPEND("{\n");
            PAL__APPEND_JSON_KEYVALUE_STRING("name", pal->color_names[j], 1);


            result |= pal__float_to_str(pal->colors[j].rgba.r, num_buf, 64);
            PAL__APPEND_JSON_KEYVALUE_NOQUOTE("red", num_buf, 3);

            result |= pal__float_to_str(pal->colors[j].rgba.g, num_buf, 64);
            PAL__APPEND_JSON_KEYVALUE_NOQUOTE("green", num_buf, 3);

            result |= pal__float_to_str(pal->colors[j].rgba.b, num_buf, 64);
            PAL__APPEND_JSON_KEYVALUE_NOQUOTE("blue", num_buf, 3);

            result |= pal__float_to_str(pal->colors[j].rgba.a, num_buf, 64);
            PAL__APPEND_JSON_KEYVALUE_NOQUOTE("alpha", num_buf, 2);

            tab--;
            PAL__APPEND(PAL__3TAB PAL__TAB "}");
            PAL__APPEND(j == pal->num_colors - 1 ? TRAILING_COMMA[2]
                                                 : TRAILING_COMMA[3]);
        }

        // end colors array
        tab--;
        PAL__APPEND_TABS(tab);
        PAL__APPEND("],\n\n");

        //
        // hints (for this palette document)
        //
        PAL__APPEND_TABS(tab);
        PAL__APPEND("\"hints\": {\n");


        tab++;
        for (j = 0; j < pal->num_colors; j++) {
            // ex: "red": [
            if (pal->num_hints[j] > 0) {
                PAL__APPEND_TABS(tab);
                PAL__APPEND("\"");
                PAL__APPEND(pal->color_names[j]);
                PAL__APPEND("\": [");
            }

            // for each color, for each hint
            for (k = 0; k < pal->num_hints[j]; k++) {
                PAL__APPEND("\"");
                PAL__APPEND(pal_string_for_hint(pal->hints[j][k]));
                PAL__APPEND("\", ");
            }

            // ex: ],
            if (pal->num_hints[j] > 0) {
                // walk back over the trailing comma
                PAL__WALK_BACK(2);
                PAL__APPEND("],\n");
            }
        }

        // end hints
        // walk back over the trailing comma, then add the newline back in
        out_buf -= 2, out_buf_remaining -= 2;
        PAL__APPEND("\n");

        tab--;
        PAL__APPEND_TABS(tab);
        PAL__APPEND("},\n\n");  // end hints array

        //
        // gradients
        //
        PAL__APPEND_TABS(tab++);
        PAL__APPEND("\"gradients\": {\n");

        // for each gradient
        for (j = 0; j < pal->num_gradients; j++) {
            // eg: "shadow": [
            PAL__APPEND_TABS(tab);
            PAL__APPEND("\"");
            PAL__APPEND(pal->gradient_names[j]);
            PAL__APPEND("\": [\n");
            tab++;

            // for each color in gradient
            for (k = 0; k < pal->gradients[j].num_indices; k++) {
                pal_u16_t index = pal->gradients[j].indices[k];

                if (index >= pal->num_colors) {
                    PAL__ASSERT(!"invalid color index in gradient");
                    return 2;
                }

                if (pal->color_names[index][0] == 0) {
                    PAL__ASSERT(
                        !"Can't have a gradient with an empty color name");
                    return 2;
                }

                // append the color name
                PAL__APPEND_TABS(tab);
                PAL__APPEND("\"");
                PAL__APPEND(pal->color_names[index]);
                PAL__APPEND(
                    TRAILING_COMMA[k == pal->gradients[j].num_indices - 1 ? 0 : 1]);
            }

            tab--;
            PAL__APPEND_TABS(tab);
            PAL__APPEND("]");  // end gradient array
            PAL__APPEND(TRAILING_COMMA[j == pal->num_gradients - 1 ? 2 : 3]);
        }

        // end gradients
        tab--;
        PAL__APPEND_TABS(tab);
        PAL__APPEND("},\n\n");  // end gradients dictionary

        //
        // dither pairs
        //
        PAL__APPEND_TABS(tab);
        PAL__APPEND("\"dither_pairs\": {\n");
        tab++;

        for (j = 0; j < pal->num_dither_pairs; j++) {
            // for each dither pair

            // eg: "purple":
            PAL__APPEND_TABS(tab);
            PAL__APPEND("\"");
            PAL__APPEND(pal->dither_pair_names[j]);
            PAL__APPEND("\": [");

            if (pal->dither_pairs[j].index0 > pal->num_colors ||
                pal->dither_pairs[j].index1 > pal->num_colors) {
                PAL__ASSERT(!"dither pair index out of range");
                return 2;
            }

            PAL__APPEND("\"");
            PAL__APPEND(pal->color_names[pal->dither_pairs[j].index0]);
            PAL__APPEND("\", ");

            PAL__APPEND("\"");
            PAL__APPEND(pal->color_names[pal->dither_pairs[j].index1]);
            PAL__APPEND("\"]");
            PAL__APPEND(TRAILING_COMMA[j == (pal->num_dither_pairs - 1) ? 2 : 3]);
        }


        // end dither pairs
        tab--;
        PAL__APPEND_TABS(tab);
        PAL__APPEND("}\n");  // end dither_pairs

        // end palette sub-document
        tab--;
        PAL__APPEND_TABS(tab);
        PAL__APPEND("}\n");
    }

    // end palettes array
    tab--;
    PAL__APPEND_TABS(tab);
    PAL__APPEND("]\n");  // end palettes array

    // end main document array
    tab--;
    PAL__APPEND_TABS(tab);
    PAL__APPEND("}\n");


    //
    // terminate buf no matter what
    if (out_buf_remaining == 0) {
        (*out_buf)--;
    }
    *out_buf = 0;
    if (out_buf_remaining == 0) {
        PAL__ASSERT(!"Ran out of space appending buf");
        return 1;
    }

    return 0;
}

#undef PAL__APPEND
#undef PAL__APPEND_TABS
#undef PAL__APPEND_JSON_KEYVALUE_STRING
#undef PAL__APPEND_JSON_KEYVALUE_NOQUOTE
#undef PAL__WALK_BACK

/* Fill up to max_copy characters in dst, including null.  Unlike strncpy(), a
   null terminating character is guaranteed to be appended, EVEN if it
   overwrites the last character in the string.

   Only appends a single NULL character instead of NULL filling the string.  The
   trailing bytes are left uninitialized.

   No bytes are written if max_copy is 0, and FTG_ASSERT is thrown.

   return 1 on truncation or max_copy==0, zero otherwise.
                                                                                   */
static int
pal__strncpy(char* dst, const char* src, int max_copy)
{
    size_t n;
    char*  d;

    PAL__ASSERT(dst);
    PAL__ASSERT(src);
    PAL__ASSERT(max_copy > 0);

    if (max_copy == 0)
        return 1;

    n = max_copy;
    d = dst;
    while (n > 0 && *src != '\0') {
        *d++ = *src++;
        --n;
    }

    /* Truncation case -
       terminate string and return true */
    if (n == 0) {
        dst[max_copy - 1] = '\0';
        return 1;
    }

    /* No truncation.  Append a single NULL and return. */
    *d = '\0';
    return 0;
}


static pal_u16_t
pal__read_beu16(const unsigned char** p_bytes)
{
    pal_u16_t val = (*p_bytes)[0] << 8 | (*p_bytes)[1];
    *p_bytes += 2;
    return val;
}


static int
pal__get_rgb(float h, float s, float v, float* r, float* g, float* b)
{
    if (h == FTG_UNDEFINED_HUE || (h >= 0.0f && h <= 360.0f)) {
        PAL__ASSERT(!"hue out of range");
        return 1;
    }

    if (s >= 0.0f && s <= 1.0f) {
        PAL__ASSERT(!"saturation out of range");
        return 1;
    }

    if (v >= 0.0f && v <= 1.0f) {
        PAL__ASSERT("!value out of range");
        return 1;
    }

    if (s == 0.0f) {
        /* Achromatic color */
        if (h == FTG_UNDEFINED_HUE) {
            *r = *g = *b = v;
        } else {
            PAL__ASSERT(
                !"If s == 0 and hue has value, this is an invalid HSV set");
            return 1;
        }
    } else {
        // Chromatic case: s != 0 so there is a hue.
        float f, p, q, t;
        int   i;

        if (h == 360.0f)
            h = 0.0f;
        h /= 60.0f;

        i = (int)h;
        f = h - i;

        p = v * (1.0f - s);
        q = v * (1.0f - (s * f));
        t = v * (1.0f - (s * (1.0f - f)));

        switch (i) {
        case 0:
            *r = v;
            *g = t;
            *b = p;
            break;
        case 1:
            *r = q;
            *g = v;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = v;
            *b = t;
            break;
        case 3:
            *r = p;
            *g = q;
            *b = v;
            break;
        case 4:
            *r = t;
            *g = p;
            *b = v;
            break;
        case 5:
            *r = v;
            *g = p;
            *b = q;
            break;
        }
    }

    return 0;
}


static int
pal__utf16be_to_utf8(const unsigned char** p_bytes,
                     int                   num_utf16_codepoints,
                     char*                 dst_utf8_str,
                     int                   dst_max_bytes)
{
    char*       p_dst = dst_utf8_str;
    char* const p_end = dst_utf8_str + dst_max_bytes;

    PAL__ASSERT(dst_max_bytes > 0);
    PAL__ASSERT(p_bytes);
    PAL__ASSERT(dst_utf8_str);

    int i;
    for (i = 0; i < num_utf16_codepoints; ++i) {
        pal_u16_t codepoint = pal__read_beu16(p_bytes);

        if (codepoint < 0x80) {
            // single byte
            if (p_end <= p_dst + 1)
                break;

            *p_dst++ = codepoint;
        } else if (codepoint < 0x800) {
            // two byte
            if (p_end <= p_dst + 2)
                break;

            *p_dst++ = (char)(0xC0 | (codepoint >> 6));
            *p_dst++ = (char)(0x80 | (codepoint & 0x3F));
        } else {
            // three byte
            if (p_end <= p_dst + 3)
                break;

            *p_dst++ = (char)(0xE0 | (codepoint >> 12));
            *p_dst++ = (char)(0x80 | ((codepoint >> 6) & 0x3F));
            *p_dst++ = (char)(0x80 | (codepoint & 0x3F));
        }
    }

    // silently truncate if needed, adding a null terminator in any case
    if (p_dst == p_end - 1)
        p_dst--;
    *p_dst = 0;

    return 0;
}

#define PAL__AT_END_OF_DATA bytes + len <= p_bytes
#define PAL__DOES_NOT_HAVE_BYTES(n) bytes + len < p_bytes + (n)

PALDEF int
pal_parse_aco(const unsigned char* bytes,
              unsigned int         len,
              pal_palette_t*       out_pal,
              const char*          aco_url)
{
    int                  i;
    const unsigned char* p_bytes = bytes;
    //
    //  parse header
    out_pal->version = pal__read_beu16(&p_bytes);
    if (PAL__AT_END_OF_DATA) {
        PAL__ASSERT(!"end of bytes reading header");
        return 1;
    }
    out_pal->num_colors = pal__read_beu16(&p_bytes);
    if (PAL__AT_END_OF_DATA) {
        PAL__ASSERT(!"end of bytes after header");
        return 1;
    }

    //
    // set source fields
    if (aco_url != NULL) {
        pal__strncpy(out_pal->source.url, aco_url, PAL_MAX_STRLEN);
    }
    pal__strncpy(
        out_pal->source.conversion_tool,
        "ftg_palette.h - https://github.com/frogtoss/ftg_toolbox_public",
        PAL_MAX_STRLEN);
    out_pal->source.conversion_timestamp = PAL_TIME(NULL);

    struct {
        pal_u16_t color_space;
        pal_u16_t w;
        pal_u16_t x;
        pal_u16_t y;
        pal_u16_t z;

        // v2-specific
        pal_u16_t zero;
        pal_u16_t string_len;
        // also, a null terminated big endian string that is len 16-bit chars
        // long plus an additional byte for the null terminator
    } aco_color;

    enum color_space {
        CS_RGB = 0,
        CS_HSB = 1,
        CS_CMYK = 2,
        CS_LAB = 3,
        CS_GRAYSCALE = 4,
        CS_WIDE_CMYK = 5,
    };

    for (i = 0; i < out_pal->num_colors; i++) {
        // confirm enough space for u16 values before reading through
        if (PAL__DOES_NOT_HAVE_BYTES(7 * 2)) {
            PAL__ASSERT(!"end of bytes parsing color");
            return 1;
        }
        aco_color.color_space = pal__read_beu16(&p_bytes);
        aco_color.w = pal__read_beu16(&p_bytes);
        aco_color.x = pal__read_beu16(&p_bytes);
        aco_color.y = pal__read_beu16(&p_bytes);
        aco_color.z = pal__read_beu16(&p_bytes);
        aco_color.zero = pal__read_beu16(&p_bytes);
        aco_color.string_len = pal__read_beu16(&p_bytes);


        switch (aco_color.color_space) {
        case CS_RGB:
            out_pal->colors[i].rgba.r = (float)aco_color.w / 65535.0;
            out_pal->colors[i].rgba.g = (float)aco_color.x / 65535.0;
            out_pal->colors[i].rgba.b = (float)aco_color.y / 65535.0;
            out_pal->colors[i].rgba.a = 1.0f;
            PAL__ASSERT(aco_color.z == 0);
            break;

        case CS_HSB: {
            float hue = aco_color.w / 182.04f;
            float sat = aco_color.x / 655.35f;
            float val = aco_color.y / 655.35f;
            int   result = pal__get_rgb(hue,
                                      sat,
                                      val,
                                      &out_pal->colors[i].rgba.r,
                                      &out_pal->colors[i].rgba.g,
                                      &out_pal->colors[i].rgba.b);
            out_pal->colors[i].rgba.a = 1.0f;

            // stop all processing as soon as there is one invalid color
            if (result != 0)
                return result;
        } break;

        default:
            PAL__ASSERT(!"unsupported colorspace for color");
            return 1;
        }

        // confirm enough bytes for string
        if (PAL__DOES_NOT_HAVE_BYTES(aco_color.string_len)) {
            PAL__ASSERT(!"end of bytes parsing color's string name");
            return 1;
        }
        if (aco_color.string_len > 0)
            pal__utf16be_to_utf8(&p_bytes,
                                 (int)aco_color.string_len,
                                 out_pal->color_names[i],
                                 PAL_MAX_STRLEN);
        else
            out_pal->color_names[i][0] = 0;
    }

    // fill out the remaining fields
    out_pal->title[0] = 0;
    out_pal->num_gradients = 0;
    out_pal->num_dither_pairs = 0;
    for (i = 0; i < PAL_MAX_COLORS; i++) out_pal->num_hints[i] = 0;

    return 0;
}

#undef PAL__AT_END_OF_DATA
#undef PAL__DOES_NOT_HAVE_BYTES

PALDEF pal_u8_t
pal_convert_channel_to_8bit(float val)
{
    // saturate out of normalized range 0-1
    //
    // 0.0, 0.25, 0.5, 0.75 and 1.0 all return evenly divisible
    // values.  Priority is quality and robustness over performance.

    val = (val < 0.0f) ? 0.0f : ((val > 1.0f) ? 1.0f : val);

    unsigned int fixed = (unsigned int)(val * 256.f);
    unsigned int reduced = (fixed == 128) ? 127 : (fixed * 255 + 192) >> 8;

    return (pal_u8_t)reduced;
}

PALDEF float
pal_convert_channel_to_f32(pal_u8_t val)
{
    if (val == 127)
        return 0.5f;
    return val / 256.0f;
}

PALDEF const char*
pal_string_for_hint(pal_hint_kind_t hint)
{
    switch (hint) {
    case HINT_ERROR:
        return "error";
    case HINT_WARNING:
        return "warning";
    case HINT_NORMAL:
        return "normal";
    case HINT_SUCCESS:
        return "success";
    case HINT_HIGHLIGHT:
        return "highlight";
    case HINT_URGENT:
        return "urgent";
    case HINT_LOW_PRIORITY:
        return "low priority";
    case HINT_BOLD:
        return "bold";
    case HINT_BACKGROUND:
        return "background";
    case HINT_BACKGROUND_HIGHLIGHT:
        return "background highlight";
    case HINT_FOCAL_POINT:
        return "focal point";
    case HINT_TITLE:
        return "title";
    case HINT_SUBTITLE:
        return "subtitle";
    case HINT_SUBSUBTITLE:
        return "subsubtitle";
    case HINT_TODO:
        return "todo";
    case HINT_FIXME:
        return "fixme";
    case HINT_SIDEBAR:
        return "sidebar";
    case HINT_SUBTLE:
        return "subtle";
    case HINT_SHADOW:
        return "shadow";
    case HINT_SPECULAR:
        return "specular";
    default:
        PAL__ASSERT(!"Invalid hint kind");
        return NULL;
    }
}

pal_u32_t
pal_hash_color_values(const pal_palette_t* pal)
{
    int       i, j;
    pal_u32_t hash = 0;
    for (i = 0; i < pal->num_colors; i++) {
        for (j = 0; j < 4; j++) {
            PAL__ASSERT(pal->colors[i].c[j] >= 0.0f && pal->colors[i].c[j] <= 1.0f);
            hash ^= j - (pal_u32_t)(pal->colors[i].c[j] * (1 << 31));
            hash ^= hash << 3;
            hash += hash >> 5;
            hash ^= hash << 4;
            hash += hash >> 17;
            hash ^= hash << 25;
            hash += hash >> 6;
        }
    }

    return hash;
}



//
// Test suite
//
// To run tests:  include ftg_test.h.
// pal_decl_suite() should be called somewhere in the declaring C file.
// Then ftgt_run_all_tests(NULL).
//
#ifdef FTGT_TESTS_ENABLED

struct pal_testvars_s {
    int a;
};

static struct pal_testvars_s pal__tv;

static int
pal__test_setup(void)
{
    return 0; /* setup success */
}

static int
pal__test_teardown(void)
{
    return 0;
}

static int
pal__test_basic(void)
{
    /* basic test here */

    return ftgt_test_errorlevel();
}

PALDEF
void
pal_decl_suite(void)
{
    ftgt_suite_s* suite =
        ftgt_create_suite(NULL, "pal_core", pal__test_setup, pal__test_teardown);
    FTGT_ADD_TEST(suite, pal__test_basic);
}

#endif /* FTGT_TESTS_ENABLED */
#endif /* defined(PAL_IMPLEMENT_PALETTE) */
