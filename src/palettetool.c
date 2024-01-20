/* palettetool Copyright (C) 2024 Frogtoss Games, Inc. */


#define FTG_IMPLEMENT_PALETTE
#define FTG_IMPLEMENT_CORE
#define KGFLAGS_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include "3rdparty/ftg_core.h"
#include "3rdparty/ftg_palette.h"
#include "3rdparty/kgflags.h"
#include "3rdparty/stb_image_write.h"
#include "3rdparty/jsmn.h"

struct args_s {
    const char* in_file;
    const char* out_file;
    bool        verbose;
    bool        help_supported;

    const char* png_sort_kind;

    int json_palette_index;
} args;

#define LOG_WARNING 1
#define LOG_MSG 0

typedef enum {
    FILE_KIND_UNKNOWN = 0,
    FILE_KIND_ACO,
    FILE_KIND_PNG,
    FILE_KIND_JSON_PALETTE,
} file_kind_t;

const file_kind_t SUPPORTED_INPUT_FORMATS[] = {FILE_KIND_ACO, FILE_KIND_JSON_PALETTE, 0};
const file_kind_t SUPPORTED_OUTPUT_FORMATS[] = {
    FILE_KIND_JSON_PALETTE, FILE_KIND_PNG, 0};

int parse_json_into_palettes(const char*    json_str,
                             usize          json_strlen,
                             pal_palette_t* out_palettes,

                             int first_palette,
                             int num_palettes,

                             char out_error_message[PAL_MAX_STRLEN],
                             int* out_error_start);

const char*
kind_to_string(file_kind_t kind)
{
    switch (kind) {
    case FILE_KIND_ACO:
        return "aco";
    case FILE_KIND_PNG:
        return "png";
    case FILE_KIND_JSON_PALETTE:
        return "json (palette format)";
    default:
        return "unknown";
    }
}

void
fatal(const char* msg)
{
    fprintf(stderr, "fatal: %s\n", msg);
    exit(1);
}

void
print(int level, const char* msg)
{
    if (level == 1 || args.verbose) {
        puts(msg);
    }
}

void
print_header(void)
{
    printf("palettetool\n\ta command-line pipeline tool to convert between "
           "palette formats\n");
    printf("Copyright (C) 2024 Frogtoss Games, Inc.\n");
}


file_kind_t
file_kind_for_extension(const char* path)
{
    const char* ext = ftg_get_filename_ext(path);

    if (*ext == '\0')
        return FILE_KIND_UNKNOWN;

    if (ftg_stricmp(ext, "aco") == 0)
        return FILE_KIND_ACO;

    if (ftg_stricmp(ext, "json") == 0)
        return FILE_KIND_JSON_PALETTE;

    if (ftg_stricmp(ext, "png") == 0)
        return FILE_KIND_PNG;

    return FILE_KIND_UNKNOWN;
}

int
add_full_palette_gradients(pal_palette_t* palette)
{
    int result =
        pal_create_sorted_gradient(palette, "sort by red channel", pal_red_cb, NULL);

    result |= pal_create_sorted_gradient(
        palette, "sort by green channel", pal_green_cb, NULL);

    result |=
        pal_create_sorted_gradient(palette, "sort by blue channel", pal_blue_cb, NULL);

    result |= pal_create_sorted_gradient(palette, "sort by hue", pal_hue_cb, NULL);

    result |= pal_create_sorted_gradient(
        palette, "sort by saturation", pal_saturation_cb, NULL);

    result |= pal_create_sorted_gradient(palette, "sort by value", pal_value_cb, NULL);

    result |= pal_create_sorted_gradient(
        palette, "sort by lightness", pal_lightness_cb, NULL);

    FTG_ASSERT(result == 0);

    return result;
}

void
print_supported_kinds(void)
{
    printf("supported input formats:\n");
    const file_kind_t* kind = &SUPPORTED_INPUT_FORMATS[0];
    while (*kind) {
        printf(" - .%s\n", kind_to_string(*kind));
        kind++;
    }

    puts("");
    printf("supported output formats:\n");
    kind = &SUPPORTED_OUTPUT_FORMATS[0];
    while (*kind) {
        printf(" - .%s\n", kind_to_string(*kind));
        kind++;
    }
}

#define SORT_BASED_ON_NAME_IF_MATCH(n)                                              \
    if (ftg_stricmp(sort_kind, #n) == 0) {                                          \
        result |= pal_create_sorted_gradient(pal, "export_me", pal_##n##_cb, NULL); \
    }

pal_gradient_t*
get_export_gradient_from_sort_kind(pal_palette_t* pal, const char* sort_kind)
{
    // common case: no sort requested by user
    int grad_idx = pal->num_gradients;
    if (!sort_kind) {
        for (int i = 0; i < pal->num_colors; i++) {
            pal->gradients[grad_idx].indices[i] = i;
        }
        strcpy(pal->gradient_names[pal->num_gradients], "export_me");
        pal->gradients[grad_idx].num_indices = pal->num_colors;
        pal->num_gradients++;
        goto end;
    }

    // this adds a gradient called "export me" in-place
    int result = 0;
    SORT_BASED_ON_NAME_IF_MATCH(red);
    SORT_BASED_ON_NAME_IF_MATCH(green);
    SORT_BASED_ON_NAME_IF_MATCH(blue);
    SORT_BASED_ON_NAME_IF_MATCH(hue);
    SORT_BASED_ON_NAME_IF_MATCH(saturation);
    SORT_BASED_ON_NAME_IF_MATCH(value);
    SORT_BASED_ON_NAME_IF_MATCH(lightness);
    FTG_ASSERT(result == 0);

end:
    return &pal->gradients[grad_idx];
}

int
main(int argc, char* argv[])
{
    kgflags_string("in", NULL, "file to convert", true, &args.in_file);
    kgflags_string("out", NULL, "file to export to (will overwrite)", true, &args.out_file);
    kgflags_bool("verbose", false, "log verbosity", false, &args.verbose);
    kgflags_string("sort-png",
                   NULL,
                   "when exporting as png, use a sort (supported: red, green, "
                   "blue, hue, saturation, value, brightness)",
                   false,
                   &args.png_sort_kind);
    kgflags_int("json-palette-index",
                0,
                "palette to parse in the json doc (starting from 0)",
                false,
                &args.json_palette_index);


    if (!kgflags_parse(argc, argv)) {
        print_header();
        kgflags_print_errors();
        kgflags_print_usage();
        print_supported_kinds();
        return 1;
    }

    if (args.help_supported) {
        print_header();
        print_supported_kinds();
        return 0;
    }

    print(LOG_MSG, ftg_va("converting '%s' to '%s'\n", args.in_file, args.out_file));

    file_kind_t in_kind = file_kind_for_extension(args.in_file);
    file_kind_t out_kind = file_kind_for_extension(args.out_file);

    //
    // read palette
    pal_palette_t palette = {0};

    switch (in_kind) {
    case FILE_KIND_ACO: {
        ftg_off_t aco_len;
        u8*       aco_bytes = ftg_file_read(args.in_file, false, &aco_len);
        if (aco_bytes == NULL)
            fatal(ftg_va("failed to read '%s'", args.in_file));

        int result = pal_parse_aco(aco_bytes, (unsigned int)aco_len, &palette, NULL);
        FTG_FREE(aco_bytes);
        if (result != 0) {
            fatal(ftg_va("failed to parse '%s'", args.in_file));
        }
    } break;

    case FILE_KIND_JSON_PALETTE: {
        ftg_off_t json_strlen;
        u8*       json_string = ftg_file_read(args.in_file, true, &json_strlen);

        char error_message[PAL_MAX_STRLEN] = {0};
        int  error_location;

        int result = parse_json_into_palettes((char*)json_string,
                                              json_strlen,
                                              &palette,

                                              args.json_palette_index,
                                              1,

                                              error_message,
                                              &error_location);
        if (result != 0) {
            fatal(ftg_va("Failed to parse json: '%s' at char offset %d",
                         error_message,
                         error_location));
        }

        FTG_FREE(json_string);
        if (palette.num_colors == 0) {
            fatal("parsed palette has 0 colors");
        }
    } break;

    default:
        fatal("Unsupported input kind. Only 'ACO' is currently supported");
    }

    //
    // write file
    switch (out_kind) {
    case FILE_KIND_JSON_PALETTE: {
        int result = add_full_palette_gradients(&palette);

        usize output_buf_bytes = (1 << 15);
        char* buf = FTG_MALLOC(sizeof(u8), output_buf_bytes);

        result = pal_emit_palette_json(&palette, 1, buf, output_buf_bytes);
        if (result != 0)
            fatal("failed to generate json palette");

        result = ftg_file_write(args.out_file, (u8*)buf, strlen(buf) + 1);
        if (result == 0)
            fatal(ftg_va("failed to write json palette to '%s'", args.out_file));

        FTG_FREE(buf);
    } break;

    case FILE_KIND_PNG: {
        // create rgba color row from palette
        u8* image_data = FTG_MALLOC(sizeof(u8), 4 * palette.num_colors);
        u8* p = image_data;

        pal_gradient_t* gradient =
            get_export_gradient_from_sort_kind(&palette, args.png_sort_kind);
        FTG_ASSERT_ALWAYS(gradient->num_indices == palette.num_colors);

        for (int i = 0; i < gradient->num_indices; i++) {
            for (int j = 0; j < 4; j++) {
                float chan32 = palette.colors[gradient->indices[i]].c[j];
                u8    chan8 = pal_convert_channel_to_8bit(chan32);
                *p++ = chan8;
            }
        }

        int result = stbi_write_png(
            args.out_file, palette.num_colors, 1, 4, image_data, palette.num_colors);
        if (result == 0) {
            fatal(ftg_va("failed to write png file to '%s'", args.out_file));
        }

        FTG_FREE(image_data);
    }

    break;

    default:
        fatal("Unsupported output kind. Only json palette is currently "
              "supported");
    }

    print(LOG_MSG, "success.");

    return 0;
}

#include "inline/parse_json.c"
