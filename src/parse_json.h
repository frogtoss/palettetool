#ifndef PARSE_JSON_H
#define PARSE_JSON_H

typedef struct pal_palette_s pal_palette_t;


int parse_json_into_palettes(
    const char*    json_str,      // json input as null terminated string
    size_t         json_strlen,   // strlen(json_str)
    pal_palette_t* out_palettes,  // pointer to pal_palette_t array

    int first_palette,  // first palette in json_str to parse
    int num_palettes,   // how many to parse into out_palettes

    char out_error_message[48],  // if return !=0, this will contain the parse error
    int* out_error_start         // index into json_str where the error started
);

#endif
