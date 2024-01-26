#ifndef PARSE_JSON_H
#define PARSE_JSON_H

typedef struct pal_palette_s pal_palette_t;

int parse_json_into_palettes(const char*    json_str,
                             size_t         json_strlen,
                             pal_palette_t* out_palettes,
                             int            first_palette,
                             int            num_palettes,

                             char out_error_message[48],
                             int* out_error_start);

#endif
