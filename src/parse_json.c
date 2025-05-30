/* palettetool Copyright (C) 2024, 2025 Frogtoss Games, Inc. */

/*
  This palette JSON parsing code is purposely written in a separate c
  file so it can be ripped out, and palette parsing can be added to
  another program without too much trouble.

  It is a fast, malloc-less parse of a json document which should
  compile warnings-free on Visual C++, GCC and Clang.  To add it:

   1. copy parse_json.c and parse_json.h to your project

   2. add jsmn.h to your project's source tree

   3. fix up the 3rdparty include paths below

   4. call parse_json_into_palettes(), returning 0 on success

  This parser depends on jsmn for tokenization.  The resulting parse
  has the following properties:

   - silent truncation of strings to fit in PAL_MAX_STRLEN

   - no utf-8 escaping of json strings

   - strict on not allowing additional keys (it will error out)

   - fails on first error with semi-vague error message, but also
     the char start of the error tokne

   - uses libc routines for string to number conversion, which
     conflate 0 and error, so silent errors to 0 can occur

   - single-pass parsing: colors array must appear and name all
     colors before any subsequent fields reference those names

   - it's fast

 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>  // for strtod
#include <assert.h>

#include "3rdparty/ftg_palette.h"
#include "3rdparty/jsmn.h"

#define JSON_ASSERT(expr) assert(expr)
#define JSON_UNUSED(x) ((void)x)

// increase this for multiple palettes in a json doc
#define MAX_JSMN_TOKENS 1024

typedef struct {
    jsmntok_t   tok[MAX_JSMN_TOKENS];
    int         num_tokens;
    const char* str;

    // set these to values that are returned to the caller
    // so functions up the stack can set errors
    char* parse_error;  // must point to string of PAL_MAX_STRLEN bytes
    int*  error_start;
} json_context_t;

#define JSON_EOF (*i == ctx->num_tokens)

// if a token, such as a key that isn't part of the spec is found,
// this returns failure for parsing the document
#define JSON_RETURN_IF_UNMATCHED_TOKEN                                         \
    if (*i == iter_start) {                                                    \
        json_error(ctx, "unexpected token", *i);                               \
        return 1;                                                              \
    }

// max_copy includes null byte
static int
json_strncpy(char* dst, const char* src, int max_copy)
{
    size_t n;
    char*  d;

    JSON_ASSERT(dst);
    JSON_ASSERT(src);
    JSON_ASSERT(max_copy > 0);

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

static int
jsoneq(json_context_t* ctx, int i, const char* s)
{
    jsmntok_t* tok = &ctx->tok[i];

    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(ctx->str + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

static void
json_skip(const json_context_t* ctx, int* i)
{
    for (int char_end = ctx->tok[*i].end;
         *i < ctx->num_tokens && ctx->tok[*i].start < char_end;
         (*i)++);  // pass
}


static void
json_strcpy_token(json_context_t* ctx, char dst[PAL_MAX_STRLEN], int i)
{
    JSON_ASSERT(ctx->tok[i].type == JSMN_STRING);

    int len_including_null = ctx->tok[i].end - ctx->tok[i].start + 1;

    // silently truncate
    if (len_including_null > PAL_MAX_STRLEN)
        len_including_null = PAL_MAX_STRLEN;

    json_strncpy(dst, ctx->str + ctx->tok[i].start, len_including_null);
}


#if 0
// keeping this around as a good example of how to recurse tokens, but
// it's not necessary after I figured out the json_skip trick

// *i will point to the next token after a dict or array, recursively skipping
// all tokens including nested dictionaries and arrays
static void
jsonskip(const json_context_t* ctx, int* i, int depth)
{
    if (*i >= ctx->num_tokens)
        return;

    const jsmntok_t* skip_tok = &ctx->tok[*i];
    if ((skip_tok->type & (JSMN_OBJECT | JSMN_ARRAY)) == 0) {
        (*i)++;
        return;
    }

    // multiply by 2 for key/value pairs
    int sz = skip_tok->type == JSMN_OBJECT ? skip_tok->size * 2 : skip_tok->size;

    for (int j = 0; j < sz; j++) {
        (*i)++;
        if (ctx->tok[*i].type == JSMN_OBJECT || ctx->tok[*i].type == JSMN_ARRAY) {
            jsonskip(ctx, i, depth + 1);
        }
    }

    if (depth == 0)
        (*i)++;
}
#endif


static void
json_error(json_context_t* ctx, const char* error, int i)
{
    json_strncpy(ctx->parse_error, error, PAL_MAX_STRLEN);
    *ctx->error_start = ctx->tok[i].start;
    // JSON_ASSERT(!"break on error");
}

// check if token is of type, setting error if it doesn't
static int
json_expect(json_context_t* ctx, jsmntype_t jsmn_type_flags, int i)
{
    JSON_ASSERT(i < ctx->num_tokens);

    if ((ctx->tok[i].type & jsmn_type_flags) != 0) {
        return 0;
    }

    json_error(ctx, "token did not match expected type", i);

    return 1;
}

// consume token if it's of types, otherwise set error and return nonzero
static int
json_match(json_context_t* ctx, jsmntype_t jsmn_type_flags, int* i)
{
    JSON_ASSERT(*i < ctx->num_tokens);

    if ((ctx->tok[*i].type & jsmn_type_flags) != 0) {
        (*i)++;
        return 0;
    }

    json_error(ctx, "token did not match expected type", *i);

    return 1;
}

// if *key is matched, expect *out_value to be a string.
// copy up to max_value_len chars into *out_value
//
// on exit, *i points to the value token regardless of whether key matched
//
// if key is not matched, *i is not advanced at all, and error == 0
//
// return 1 on error
static int
json_write_value_str_on_match_key(
    json_context_t* ctx, const char* key, int* i, char* out_value, int max_value_len)
{
    JSON_ASSERT(max_value_len <= PAL_MAX_STRLEN);
    JSON_UNUSED(max_value_len);

    if (jsoneq(ctx, *i, key) == 0) {
        json_skip(ctx, i);

        // value for this key must be a string
        if (json_expect(ctx, JSMN_STRING, *i) != 0) {
            json_error(ctx, "expected string token type", *i);
            return 1;
        }

        json_strcpy_token(ctx, out_value, *i);
    }

    return 0;
}


// token pointed to by *i is a string that is the name of a color in
// pal->color_names. return the index to it.
static int
json_token_to_palette_color_index(json_context_t*      ctx,
                                  int                  i,
                                  const pal_palette_t* pal,
                                  int*                 out_index)
{
    const jsmntok_t* tok = &ctx->tok[i];
    if (json_expect(ctx, JSMN_STRING, i) != 0)
        return 1;

    for (int j = 0; j < pal->num_colors; j++) {
        if (strncmp(ctx->str + tok->start, pal->color_names[j], tok->end - tok->start) ==
            0) {
            *out_index = j;
            return 0;
        }
    }

    return 1;
}


// as above, but for floats
// out_inc_on_match is incremented if the key is matched
static int
json_write_value_float_on_match_key(json_context_t* ctx,
                                    const char*     key,
                                    int*            i,
                                    float*          out_float,
                                    int*            out_inc_on_match)
{
    if (jsoneq(ctx, *i, key) == 0) {
        json_skip(ctx, i);

        (*out_inc_on_match)++;

        if (json_expect(ctx, JSMN_PRIMITIVE, *i) != 0) {
            json_error(ctx, "expected primitive token type", *i);
            return 1;
        }

        *out_float = (float)strtod(ctx->str + ctx->tok[*i].start, NULL);
        // can't distinguish between 0.0 and error conversion error
    }

    return 0;
}


static int
json_write_value_ull_on_match_key(json_context_t*     ctx,
                                  const char*         key,
                                  int*                i,
                                  unsigned long long* out_num)
{
    if (jsoneq(ctx, *i, key) == 0) {
        json_skip(ctx, i);

        // the one ull we have in this document is intentionally stored inside a string
        if (json_expect(ctx, JSMN_STRING, *i) != 0) {
            json_error(ctx, "expected primitive token type", *i);
            return 1;
        }


        *out_num = strtoull(ctx->str + ctx->tok[*i].start, NULL, 10);
    }

    return 0;
}

static int
parse_palette_source_subobject(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    int obj_end_index = ctx->tok[*i].end;

    json_match(ctx, JSMN_OBJECT, i);  // skip into object's first token

    for (; !JSON_EOF && ctx->tok[*i].start < obj_end_index; (*i)++) {
        const int iter_start = *i;

        if (*i == iter_start &&
            json_write_value_str_on_match_key(
                ctx, "conversion_tool", i, pal->source.conversion_tool, PAL_MAX_STRLEN) != 0)
            return 1;

        if (*i == iter_start && json_write_value_str_on_match_key(
                                    ctx, "url", i, pal->source.url, PAL_MAX_STRLEN) != 0)
            return 1;

        if (*i == iter_start &&
            json_write_value_ull_on_match_key(
                ctx, "conversion_date", i, &pal->source.conversion_timestamp) != 0)
            return 1;

        JSON_RETURN_IF_UNMATCHED_TOKEN;
    }

    // on success, index into last element of subobject
    (*i)--;

    return 0;
}

static int
parse_color_space_subobject(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    int obj_end_index = ctx->tok[*i].end;

    json_match(ctx, JSMN_OBJECT, i);  // skip into object's first token

    for (; !JSON_EOF && ctx->tok[*i].start < obj_end_index; (*i)++) {
        const int iter_start = *i;

        if (*i == iter_start &&
            json_write_value_str_on_match_key(
                ctx, "name", i, pal->color_space.name, PAL_MAX_STRLEN) != 0)
            return 1;

        if (*i == iter_start &&
            json_write_value_str_on_match_key(
                ctx, "icc_filename", i, pal->color_space.icc_filename, PAL_MAX_STRLEN) != 0)
            return 1;

        if (*i == iter_start && jsoneq(ctx, *i, "is_linear") == 0) {
            json_skip(ctx, i);

            if (json_expect(ctx, JSMN_PRIMITIVE, *i) != 0) {
                json_error(ctx, "expected boolean (primitive) type", *i);
                return 1;
            }

            // only boolean in this whole parse
            int len = ctx->tok[*i].end - ctx->tok[*i].start;

            if (len == 4 && strncmp(ctx->str + ctx->tok[*i].start, "true", 4) == 0) {
                pal->color_space.is_linear = 1;
            } else if (len == 5 &&
                       strncmp(ctx->str + ctx->tok[*i].start, "false", 5) == 0) {
                pal->color_space.is_linear = 0;
            } else {
                json_error(ctx, "failed to parse is_linear as a boolean", *i);
                return 1;
            }
        }
    }

    // on success, index into last element of subobject
    (*i)--;

    return 0;
}

static int
parse_palette_color_subobject(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    int obj_end_index = ctx->tok[*i].end;

    // match into individual color subobject
    if (json_match(ctx, JSMN_OBJECT, i))
        return 1;


    pal_color_t* col = &pal->colors[pal->num_colors];

    JSON_ASSERT(pal->num_colors < PAL_MAX_COLORS);

    int channel_set_count = 0;
    for (; !JSON_EOF && ctx->tok[*i].start < obj_end_index; (*i)++) {
        const int iter_start = *i;

        if (*i == iter_start &&
            json_write_value_str_on_match_key(
                ctx, "name", i, pal->color_names[pal->num_colors], PAL_MAX_STRLEN) != 0)
            return 1;

        if (*i == iter_start) {
            if (json_write_value_float_on_match_key(
                    ctx, "red", i, &col->rgba.r, &channel_set_count) != 0)
                return 1;
        }

        if (*i == iter_start) {
            if (json_write_value_float_on_match_key(
                    ctx, "green", i, &col->rgba.g, &channel_set_count) != 0)
                return 1;
        }

        if (*i == iter_start) {
            if (json_write_value_float_on_match_key(
                    ctx, "blue", i, &col->rgba.b, &channel_set_count) != 0)
                return 1;
        }

        if (*i == iter_start) {
            if (json_write_value_float_on_match_key(
                    ctx, "alpha", i, &col->rgba.a, &channel_set_count) != 0)
                return 1;
        }

        // if i hasn't advanced in this loop, it is an unmatched token.
        if (*i == iter_start) {
            json_error(ctx, "unexpected token", *i);
            return 1;
        }
    }

    if (channel_set_count != 4) {
        json_error(ctx, "color does not have 4 channels", *i);
        return 1;
    }

    if (pal->color_names[pal->num_colors][0] == 0) {
        json_error(ctx, "empty or no color name for color", *i);
        return 1;
    }

    if (!JSON_EOF && ctx->tok[*i].start < obj_end_index) {
        json_error(ctx, "invalid token type found in color subobject", *i);
        return 1;
    }

    pal->num_colors++;

    (*i)--;
    return 0;
}

static int
parse_palette_colors_subarray(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    int colors_array_tokens = ctx->tok[*i].size;
    int array_end_index = ctx->tok[*i].end;

    if (json_match(ctx, JSMN_ARRAY, i) != 0)
        return 1;

    if (colors_array_tokens > PAL_MAX_COLORS) {
        json_error(ctx, "PAL_MAX_COLORS exceeded", *i);
        return 1;
    }

    pal->num_colors = 0;

    while (!JSON_EOF && ctx->tok[*i].type == JSMN_OBJECT) {
        if (parse_palette_color_subobject(ctx, i, pal) != 0)
            return 1;
        (*i)++;
    }


    if (!JSON_EOF && ctx->tok[*i].start < array_end_index) {
        // iterator still in subarray -- something other than an
        // object was found.
        json_error(ctx, "invalid token type found in colors array", *i);
        return 1;
    }

    (*i)--;

    return 0;
}

static int
parse_palette_hints_subarray(json_context_t* ctx, int* i, pal_palette_t* pal, int palette_color_index)
{
    JSON_ASSERT(palette_color_index < pal->num_colors);

    // expect an array of strings
    int hints_array_end = ctx->tok[*i].end;
    if (json_match(ctx, JSMN_ARRAY, i) != 0)
        return 1;

    for (; !JSON_EOF && ctx->tok[*i].start < hints_array_end; (*i)++) {
        if (json_expect(ctx, JSMN_STRING, *i) != 0)
            return 1;

        if (pal->num_hints[palette_color_index] >= PAL_MAX_HINTS) {
            json_error(ctx, "PAL_MAX_HINTS exceeded for color", *i);
            return 1;
        }

        // parse hint as string and turn it into a enum
        pal_hint_kind_t hint;
        int             len = ctx->tok[*i].end - ctx->tok[*i].start;
        int result = pal_hint_for_string(ctx->str + ctx->tok[*i].start, len, &hint);
        if (result != 0) {
            json_error(ctx, "invalid hint", *i);
            return 1;
        }

        // assign hint
        pal->hints[palette_color_index][pal->num_hints[palette_color_index]++] = hint;
    }

    (*i)--;

    return 0;
}

static int
parse_palette_hints_subobject(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    int hints_object_end = ctx->tok[*i].end;

    if (json_match(ctx, JSMN_OBJECT, i) != 0)
        return 1;

    // initialize all colors to zero hints
    for (int j = 0; j < PAL_MAX_COLORS; j++) pal->num_hints[j] = 0;

    // expect string: array pairs
    while (!JSON_EOF && ctx->tok[*i].start < hints_object_end) {
        int palette_color_index;

        int result =
            json_token_to_palette_color_index(ctx, *i, pal, &palette_color_index);
        if (result != 0) {
            json_error(ctx, "hints names a color name not in colors array", *i);
            return 1;
        }

        json_match(ctx, JSMN_STRING, i);
        if (parse_palette_hints_subarray(ctx, i, pal, palette_color_index) != 0)
            return 1;
        (*i)++;
    }

    (*i)--;

    return 0;
}

static int
parse_palette_gradients_subarray(json_context_t* ctx, int* i, pal_palette_t* pal, int gradient_index)
{
    int gradients_array_end = ctx->tok[*i].end;
    if (json_match(ctx, JSMN_ARRAY, i) != 0)
        return 1;

    pal->gradients[gradient_index].num_indices = 0;

    while (!JSON_EOF && ctx->tok[*i].start < gradients_array_end) {
        int palette_color_index;

        int result =
            json_token_to_palette_color_index(ctx, *i, pal, &palette_color_index);
        if (result != 0) {
            json_error(ctx, "gradient names a color name not in colors array", *i);
            return 1;
        }

        pal->gradients[gradient_index].indices[pal->gradients[gradient_index].num_indices++] =
            (pal_u16_t)palette_color_index;

        (*i)++;
    }

    (*i)--;

    return 0;
}


static int
parse_palette_gradients_subobject(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    int gradients_object_end = ctx->tok[*i].end;

    pal->num_gradients = 0;

    if (json_match(ctx, JSMN_OBJECT, i) != 0)
        return 1;

    while (!JSON_EOF && ctx->tok[*i].start < gradients_object_end) {
        if (json_expect(ctx, JSMN_STRING, *i) != 0)
            return 1;

        if (pal->num_gradients >= PAL_MAX_GRADIENTS) {
            json_error(ctx, "PAL_MAX_GRADIENTS exceeded", *i);
            return 1;
        }

        json_strcpy_token(ctx, pal->gradient_names[pal->num_gradients], *i);

        json_match(ctx, JSMN_STRING, i);
        if (parse_palette_gradients_subarray(ctx, i, pal, pal->num_gradients) != 0)
            return 1;

        pal->num_gradients++;

        (*i)++;
    }

    (*i)--;

    return 0;
}

static int
parse_palette_dither_pairs_subarray(json_context_t* ctx, int* i, pal_palette_t* pal, int dither_pair_index)
{
    if (ctx->tok[*i].size != 2) {
        json_error(ctx, "dither pairs array expects exactly 2 color names", *i);
        return 1;
    }

    if (json_match(ctx, JSMN_ARRAY, i) != 0)
        return 1;

    int pair[2];

    if (json_token_to_palette_color_index(ctx, *i, pal, &pair[0]) != 0) {
        json_error(ctx, "dither pair unknown color name", *i);
        return 1;
    }
    json_skip(ctx, i);

    if (json_token_to_palette_color_index(ctx, *i, pal, &pair[1]) != 0) {
        json_error(ctx, "dither pair unknown color name", *i);
        return 1;
    }

    pal->dither_pairs[dither_pair_index].index0 = (pal_u16_t)pair[0];
    pal->dither_pairs[dither_pair_index].index1 = (pal_u16_t)pair[1];

    // don't skip -- i remains on the last token on the array by convention
    // json_skip(ctx, i);

    return 0;
}

static int
parse_palette_dither_pairs_subobject(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    int dither_pairs_object_end = ctx->tok[*i].end;

    if (json_match(ctx, JSMN_OBJECT, i) != 0)
        return 1;

    pal->num_dither_pairs = 0;

    while (!JSON_EOF && ctx->tok[*i].start < dither_pairs_object_end) {
        if (json_expect(ctx, JSMN_STRING, *i) != 0)
            return 1;

        json_strcpy_token(ctx, pal->dither_pair_names[pal->num_dither_pairs], *i);

        json_match(ctx, JSMN_STRING, i);
        if (parse_palette_dither_pairs_subarray(ctx, i, pal, pal->num_dither_pairs) != 0)
            return 1;

        pal->num_dither_pairs++;

        (*i)++;
    }

    (*i)--;

    return 0;
}

static int
parse_palette_object(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    int object_end_char_index = ctx->tok[*i].end;

    if (json_match(ctx, JSMN_OBJECT, i) != 0)
        return 1;

    for (; !JSON_EOF && ctx->tok[*i].start <= object_end_char_index; (*i)++) {
        const int iter_start = *i;  // i == iter_start: no token consumption yet

        if (*i == iter_start && json_write_value_str_on_match_key(
                                    ctx, "title", i, pal->title, PAL_MAX_STRLEN) != 0)
            return 1;

        if (*i == iter_start && jsoneq(ctx, *i, "color_hash") == 0)
            (*i)++;  // acceptable key/value, but has no analog field

        if (*i == iter_start && jsoneq(ctx, *i, "source") == 0) {
            json_skip(ctx, i);
            if (json_expect(ctx, JSMN_OBJECT, *i) != 0)
                return 1;

            if (parse_palette_source_subobject(ctx, i, pal) != 0)
                return 1;
        }

        if (*i == iter_start && jsoneq(ctx, *i, "color_space") == 0) {
            json_skip(ctx, i);
            if (json_expect(ctx, JSMN_OBJECT, *i) != 0)
                return 1;

            if (parse_color_space_subobject(ctx, i, pal) != 0)
                return 1;
        }

        if (*i == iter_start && jsoneq(ctx, *i, "colors") == 0) {
            json_skip(ctx, i);
            if (json_expect(ctx, JSMN_ARRAY, *i) != 0)
                return 1;

            if (parse_palette_colors_subarray(ctx, i, pal) != 0)
                return 1;
        }

        if (*i == iter_start && jsoneq(ctx, *i, "hints") == 0) {
            json_skip(ctx, i);
            if (json_expect(ctx, JSMN_OBJECT, *i) != 0)
                return 1;

            if (parse_palette_hints_subobject(ctx, i, pal) != 0)
                return 1;
        }

        if (*i == iter_start && jsoneq(ctx, *i, "gradients") == 0) {
            json_skip(ctx, i);

            if (json_expect(ctx, JSMN_OBJECT, *i) != 0)
                return 1;

            if (parse_palette_gradients_subobject(ctx, i, pal) != 0)
                return 1;
        }

        if (*i == iter_start && jsoneq(ctx, *i, "dither_pairs") == 0) {
            json_skip(ctx, i);

            if (json_expect(ctx, JSMN_OBJECT, *i) != 0)
                return 1;

            if (parse_palette_dither_pairs_subobject(ctx, i, pal) != 0)
                return 1;
        }

        JSON_RETURN_IF_UNMATCHED_TOKEN;
    }

    (*i)--;

    return 0;
}

int
parse_json_into_palettes(const char*    json_str,
                         size_t         json_strlen,
                         pal_palette_t* out_palettes,
                         int            first_palette,
                         int            num_palettes,

                         char out_error_message[PAL_MAX_STRLEN],
                         int* out_error_start)
{
    jsmn_parser    parser;
    json_context_t ctx;

    jsmn_init(&parser);
    ctx.num_tokens =
        jsmn_parse(&parser, json_str, json_strlen, ctx.tok, MAX_JSMN_TOKENS);
    if (ctx.num_tokens < 0) {
        JSON_ASSERT(!"jsmn_parse failed with error");
        return 1;
    }
    ctx.str = json_str;
    ctx.parse_error = out_error_message;
    ctx.error_start = out_error_start;

    // expect outer object
    int i = 0;
    if (json_match(&ctx, JSMN_OBJECT, &i) != 0)
        return 1;

    // scan for 'palettes: [', setting i to the beginning of the palettes array
    for (; i < ctx.num_tokens; i++) {
        switch (ctx.tok[i].type) {
        case JSMN_STRING:
            if (jsoneq(&ctx, i, "palettes") == 0) {
                if (i < ctx.num_tokens - 1 && ctx.tok[i + 1].type == JSMN_ARRAY) {
                    // found the palettes array
                    // jump i ahead to the first palette object
                    i += 2;
                }
            }
            goto end_search;
        default:;
        }
    }
end_search:
    if (i == ctx.num_tokens) {
        JSON_ASSERT(!"json document didn't have any palettes");
        return 1;
    }

    // FTG_ASSERT(tok[i].type == JSMN_OBJECT);
    for (int current_palette = 0; current_palette != first_palette; current_palette++) {
        json_skip(&ctx, &i);
    }

    if (i == ctx.num_tokens) {
        json_error(&ctx, "out of tokens while parsing palette", i);
        return 1;
    }

    for (int pal_index = 0; pal_index < num_palettes; pal_index++) {
        pal_init(&out_palettes[pal_index]);
        if (parse_palette_object(&ctx, &i, &out_palettes[pal_index]) != 0)
            return 1;
    }

    return 0;
}
