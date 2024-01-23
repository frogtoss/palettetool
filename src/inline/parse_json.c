/* palettetool Copyright (C) 2024 Frogtoss Games, Inc. */


#if !ACTUAL_BUILD
#    include <stdio.h>
#    include <string.h>
#    include "../3rdparty/ftg_core.h"
#    include "../3rdparty/ftg_palette.h"
#endif

#include "../3rdparty/jsmn.h"

#include <stdlib.h>  // for strtod, hope to replace

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

static void
json_error(json_context_t* ctx, const char* error, int i)
{
    pal__strncpy(ctx->parse_error, error, PAL_MAX_STRLEN);
    *ctx->error_start = ctx->tok[i].start;
}

// check if token is of type, setting error if it doesn't
static int
json_expect(json_context_t* ctx, jsmntype_t jsmn_type_flags, int i)
{
    FTG_ASSERT(i < ctx->num_tokens);

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
    FTG_ASSERT(*i < ctx->num_tokens);

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
    if (jsoneq(ctx, *i, key) == 0) {
        jsonskip(ctx, i, 0);  // step past key

        // value for this key must be a string
        if (json_expect(ctx, JSMN_STRING, *i) != 0) {
            json_error(ctx, "expected string token type", *i);
            return 1;
        }

        int len = FTG_MIN(ctx->tok[*i].end - ctx->tok[*i].start + 1, max_value_len);
        pal__strncpy(out_value, ctx->str + ctx->tok[*i].start, len);
    }

    return 0;
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
        jsonskip(ctx, i, 0);

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

#ifdef DEBUG
#    define JSON_ATTRIBS "start: %d end: %d size %d\n"
static void
jsondumptok(json_context_t* ctx, int i)
{
    jsmntok_t* tok = &ctx->tok[i];
    printf("token %d\n", i);
    switch (tok->type) {
    case JSMN_UNDEFINED:
        printf("tok undefined\n");
        break;
    case JSMN_OBJECT:
        printf("tok object " JSON_ATTRIBS, tok->start, tok->end, tok->size);
        break;
    case JSMN_ARRAY:
        printf("tok array " JSON_ATTRIBS, tok->start, tok->end, tok->size);
        break;

    case JSMN_STRING:
        printf("tok string " JSON_ATTRIBS, tok->start, tok->end, tok->size);
        printf("contents: '%.*s'\n", tok->end - tok->start, ctx->str + tok->start);
        break;

    case JSMN_PRIMITIVE:
        printf("tok primitive " JSON_ATTRIBS, tok->start, tok->end, tok->size);
        break;

    default:
        FTG_ASSERT(!"fail");
    }
}
#endif



int
parse_palette_source_subobject(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    int source_subdoc_tokens = ctx->tok[*i].size * 2;

    json_match(ctx, JSMN_OBJECT, i);  // skip into object's first token
    int last = *i + source_subdoc_tokens;
    for (; *i < last; (*i)++) {
        const int iter_start = *i;

        if (*i == iter_start &&
            json_write_value_str_on_match_key(
                ctx, "conversion_tool", i, pal->source.conversion_tool, PAL_MAX_STRLEN) != 0)
            return 1;

        if (*i == iter_start && json_write_value_str_on_match_key(
                                    ctx, "url", i, pal->source.url, PAL_MAX_STRLEN) != 0)
            return 1;

        // todo: next: parse this into a u32
        char fixme_buf[PAL_MAX_STRLEN];
        if (*i == iter_start &&
            json_write_value_str_on_match_key(
                ctx, "conversion_date", i, fixme_buf, PAL_MAX_STRLEN) != 0)
            return 1;


        // if i hasn't advanced in this loop, it is an unmatched token.
        if (*i == iter_start) {
            printf("unmatched token:\n");
            jsondumptok(ctx, *i);
            puts("");
            json_error(ctx, "unexpected token", *i);
            return 1;
        }
    }

    // on success, index into last element of subobject
    (*i)--;

    return 0;
}

static int
parse_palette_color_subobject(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    int color_object_tokens = ctx->tok[*i].size * 2;
    int last = *i + color_object_tokens;

    // match into individual color subobject
    if (json_match(ctx, JSMN_OBJECT, i))
        return 1;


    pal_color_t* col = &pal->colors[pal->num_colors];

    FTG_ASSERT(pal->num_colors < PAL_MAX_COLORS);

    int channel_set_count = 0;
    for (; *i < last; (*i)++) {
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

    pal->num_colors++;

    (*i)--;
    return 0;
}

static int
parse_palette_colors_subarray(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    int colors_array_tokens = ctx->tok[*i].size;

    if (json_match(ctx, JSMN_ARRAY, i) != 0)
        return 1;

    if (colors_array_tokens > PAL_MAX_COLORS) {
        json_error(ctx, "PAL_MAX_COLORS exceeded", *i);
        return 1;
    }

    pal->num_colors = 0;

    while (ctx->tok[*i].type == JSMN_OBJECT) {
        if (parse_palette_color_subobject(ctx, i, pal) != 0)
            return 1;
        (*i)++;
    }

    (*i)--;

    return 0;
}

static int
parse_palette_hints_subarray(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    if (json_match(ctx, JSMN_ARRAY, i) != 0)
        return 1;

    // expect an array of strings
    int hints_subarray_tokens = ctx->tok[*i].size;
    int last = *i + hints_subarray_tokens;

    for (; *i != last; (*i)++) {
        if (json_expect(ctx, JSMN_STRING, *i) != 0)
            return 1;

        // todo: something with string
    }

    return 0;
}

static int
parse_palette_hints_subobject(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    int hints_subdoc_tokens = ctx->tok[*i].size * 2;

    if (json_match(ctx, JSMN_OBJECT, i) != 0)
        return 1;

    if (hints_subdoc_tokens > pal->num_colors) {
        json_error(ctx, "more hints than palette colors", *i);
        return 1;
    }

    // initialize all colors to zero hints
    for (int j = 0; j < PAL_MAX_COLORS; j++) pal->num_hints[j] = 0;

    // expect string: array pairs
    while (ctx->tok[*i].type == JSMN_STRING) {
        jsonskip(ctx, i, 0);
        if (parse_palette_hints_subarray(ctx, i, pal) != 0)
            return 1;
        (*i)++;
    }


    (*i)--;

    return 0;
}

static int
parse_palette_object(json_context_t* ctx, int* i, pal_palette_t* pal)
{
    int palette_subdoc_tokens = ctx->tok[*i].size * 2;
    int last = *i + palette_subdoc_tokens;  // fixme: definitely not last. Subobject tokens make sure of that.

    if (json_match(ctx, JSMN_OBJECT, i) != 0)
        return 1;

    for (; *i < last; (*i)++) {
        const int iter_start = *i;  // i == iter_start: no token consumption yet

        if (*i == iter_start && json_write_value_str_on_match_key(
                                    ctx, "title", i, pal->title, PAL_MAX_STRLEN) != 0)
            return 1;

        if (*i == iter_start && jsoneq(ctx, *i, "color_hash") == 0)
            (*i)++;  // acceptable key/value, but has no analog field

        if (*i == iter_start && jsoneq(ctx, *i, "source") == 0) {
            jsonskip(ctx, i, 0);
            if (json_expect(ctx, JSMN_OBJECT, *i) != 0)
                return 1;

            if (parse_palette_source_subobject(ctx, i, pal) != 0)
                return 1;
        }

        if (*i == iter_start && jsoneq(ctx, *i, "colors") == 0) {
            jsonskip(ctx, i, 0);
            if (json_expect(ctx, JSMN_ARRAY, *i) != 0)
                return 1;

            if (parse_palette_colors_subarray(ctx, i, pal) != 0)
                return 1;
        }

        if (*i == iter_start && jsoneq(ctx, *i, "hints") == 0) {
            jsonskip(ctx, i, 0);
            if (json_expect(ctx, JSMN_ARRAY, *i) != 0)
                return 1;

            if (parse_palette_hints_subobject(ctx, i, pal) != 0) {
                return 1;
            }
        }

        // if i hasn't advanced in this loop, it is an unmatched token.
        if (*i == iter_start) {
            printf("unmatched token:\n");
            jsondumptok(ctx, *i);
            puts("");
            json_error(ctx, "unexpected token", *i);
            return 1;
        }
    }

    (*i)--;

    return 0;
}

int
parse_json_into_palettes(const char*    json_str,
                         usize          json_strlen,
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
        FTG_ASSERT(!"jsmn_parse failed with error");
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
        FTG_ASSERT(!"json document didn't have any palettes");
        return 1;
    }

    // FTG_ASSERT(tok[i].type == JSMN_OBJECT);
    for (int current_palette = 0; current_palette != first_palette; current_palette++) {
        jsonskip(&ctx, &i, 0);
    }

    if (i == ctx.num_tokens) {
        json_error(&ctx, "out of tokens while parsing palette", i);
        return 1;
    }

    // parse single palette
    int            pal_num = 0;
    pal_palette_t* pal = &out_palettes[pal_num];  // todo: zero initialize
    int            result = parse_palette_object(&ctx, &i, pal);

    // todo: parse multiple palettes
    if (result != 0)
        return 1;

    return 0;
}
