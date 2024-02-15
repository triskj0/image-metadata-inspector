#ifdef _WIN32
#   define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "jpg_operations.h"
#define HEX_MULTIPLIER 256

static int start_bytes[] = {255, 216};
static char tiff_alignment[3];
static int ifd_offset;


static int _get_4_byte_int(FILE *image_file) {
    int c;
    int total = 0;

    for (int i = 4; i > 0; i--) {
        c = fgetc(image_file);

        switch (i) {
            case 4:
                total += c * HEX_MULTIPLIER * HEX_MULTIPLIER * HEX_MULTIPLIER;
                break;

            case 3:
                total += c * HEX_MULTIPLIER * HEX_MULTIPLIER;
                break;

            case 2:
                total += c * HEX_MULTIPLIER;
                break;

            case 1:
                total += c;
        }
    }
    return total;
}


static void _find_bytes_by_numeric_value(FILE *image_file, int value1, int value2) {
    int c;

    while ((c = fgetc(image_file)) != EOF) {
        if (c == value1) {
            if (fgetc(image_file) == value2) {
                return;
            }
            fseek(image_file, -1, SEEK_CUR);
        }
    }
    fprintf(stderr, "[ERROR] could not find value by number\n" \
            "%s : %d\nvalues that were expected to be found: %d, %d", __FILE__, __LINE__, value1, value2);

}


static void _reset_file_pointer(FILE *image_file) {
    fseek(image_file, 0, SEEK_SET);
    _find_bytes_by_numeric_value(image_file, start_bytes[0], start_bytes[1]);
}


static bool _find_string_in_file(FILE *image_file, char *str) {
    _reset_file_pointer(image_file);

    int str_char1 = str[0];
    int str_len = strlen(str);
    int file_char;

    while ((file_char = fgetc(image_file)) != EOF) {

        if (file_char == str_char1) {
            for (int i = 1; i < str_len; i++) {
                file_char = fgetc(image_file);

                if (file_char != str[i]) {
                    fseek(image_file, -i, SEEK_CUR);
                    break;
                }

                else if (i == str_len-1) {
                    return true;
                }
            }
        }
    }

    return false;
}


static void _read_tiff_header(FILE *image_file)
{
    if (!_find_string_in_file(image_file, "Exif")) return;

    fseek(image_file, 2, SEEK_CUR);   // skip 2x 0x00
    if (fgetc(image_file) == 'M') {   // Motorola
        strcpy(tiff_alignment, "MM");
    }
    else {
        strcpy(tiff_alignment, "II"); // Intel
    }

    fseek(image_file, 3, SEEK_CUR);   // skip the next I or M and 0x002A or 0x2A00
    ifd_offset = _get_4_byte_int(image_file);
}


void jpg_print_exif_data(FILE *image_file)
{
    _read_tiff_header(image_file);
    printf("tiff alignment: %s\n", tiff_alignment);
    printf("ifd offset: %d\n", ifd_offset);

}

