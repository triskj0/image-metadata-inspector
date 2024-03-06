#ifdef _WIN32
#   define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "jpg_operations.h"
#include "csv_lookup.h"
#define HEX_MULTIPLIER        16
#define HEX_MULTIPLIER_POW_2 256
#define MAX_PRINTED_COMMENTS  10
#define JPG_EXIF_TAGS_FILEPATH          "./csv/jpg_exif_tags.csv"
#define EXIF_COMPRESSION_TAGS_FILEPATH  "./csv/exif_compression_tags.csv"
#define CUSTOM_RENDERED_TAGS_FILEPATH   "./csv/custom_rendered_tags.csv"
#define ORIENTATION_TAGS_FILEPATH       "./csv/orientation_tags.csv"
#define SCENE_CAPTURE_TAGS_FILEPATH     "./csv/scene_capture_tags.csv"
#define RESOLUTION_UNIT_TAGS_FILEPATH   "./csv/resolution_unit_tags.csv"
#define YCBCR_POSITIONING_TAGS_FILEPATH "./csv/ycbcr_positioning_tags.csv"
#define EXPOSURE_MODE_TAGS_FILEPATH     "./csv/exposure_mode_tags.csv"
#define GAIN_CONTROL_TAGS_FILEPATH      "./csv/gain_control_tags.csv"
#define CONTRAST_TAGS_FILEPATH          "./csv/contrast_tags.csv"

static char byte_alignment[3];
static int byte_alignment_offset;
static int next_ifd_offset;
static bool jfif_is_resent = false;


typedef struct {
    int *tags;
    size_t tags_count;
    size_t capacity;
} Exif_Tag_Array;


#define tags_append(arr, tag)\
    do {\
        if (arr.tags_count >= arr.capacity) {\
            if (arr.capacity == 0) arr.capacity = 256;\
            else arr.capacity *= 2;\
            arr.tags = realloc(arr.tags, arr.capacity * sizeof(*arr.tags));\
        }\
        arr.tags[arr.tags_count++] = tag;\
    } while (0)


static bool _tag_has_occured(Exif_Tag_Array arr, int tag)
{
    for (size_t i = 0; i < arr.tags_count; i++) {
        if (arr.tags[i] == tag) return true;
    }
    return false;
}


static int _get_nth_power(int number, int power)
{
    if (power == 0) {
        return 1;
    }

    else if (power == 1) {
        return number;
    }

    int original_number = number;
    for (int i = power; i > 1; i--) {
        number *= original_number;
    }
    return number;
}


static int _get_single_digit_from_byte(int first_or_second, int byte)
{
    if (first_or_second == 1) {
        return (byte - (byte % HEX_MULTIPLIER)) / HEX_MULTIPLIER;
    }

    else if (first_or_second == 2) {
        return (byte % HEX_MULTIPLIER);
    }

    return -1;
}


static int _read_n_byte_int(FILE *image_file, int n)
{
    int c;
    int total = 0;

    if (strcmp(byte_alignment, "MM") == 0) {
        for (int i = n; i > 0; i--) {
            c = fgetc(image_file);

            total += _get_single_digit_from_byte(1, c) * _get_nth_power(HEX_MULTIPLIER, i*2-1);
            total += _get_single_digit_from_byte(2, c) * _get_nth_power(HEX_MULTIPLIER, i*2-2);
        }

        return total;
    }

    for (int i = 0; i < n; i++) {
        c = fgetc(image_file);

        total += _get_single_digit_from_byte(1, c) * _get_nth_power(HEX_MULTIPLIER, i*2+1);
        total += _get_single_digit_from_byte(2, c) * _get_nth_power(HEX_MULTIPLIER, i*2);
    }

    return total;
}


static int _read_2_byte_data_value(FILE *image_file)
{
    char c1 = fgetc(image_file);
    char c2 = fgetc(image_file);

    if (c1 > 0 && c2 > 0) {
        fseek(image_file, -2, SEEK_CUR);
        return _read_n_byte_int(image_file, 2);
    }
    else if (c1 > 0) {
        return c1;
    }
    return c2;
}


// data values sometimes only store data in the first n out of the 4
// available bytes, so reading all 4 can result in incorrect data
static int _read_4_byte_data_value(FILE *image_file)
{
    if (strcmp(byte_alignment, "II") == 0) {
        return _read_n_byte_int(image_file, 4);
    }

    int val, val2;

    val = _read_n_byte_int(image_file, 2);
    val2 = _read_n_byte_int(image_file, 2);

    if (val > 0 && val2 > 0) {
        fseek(image_file, -4, SEEK_CUR);
        return _read_n_byte_int(image_file, 4);
    }

    if (val == 0) {
        return val2;
    }

    return val;
}


static void _goto_file_offset(FILE *file, int offset)
{
    fseek(file, offset + byte_alignment_offset, SEEK_SET);
}


static int _get_string_offset_from_start_of_file(FILE *image_file, char *str)
{
    fseek(image_file, 0, SEEK_SET);

    int offset = 0;
    int str_char1 = str[0];
    int str_len = strlen(str);
    int file_char;

    while ((file_char = fgetc(image_file)) != EOF) {
        offset++;

        if (file_char == str_char1) {
            for (int i = 1; i < str_len; i++) {
                file_char = fgetc(image_file);
                offset++;

                if (file_char != str[i]) {
                    fseek(image_file, -i, SEEK_CUR);
                    offset -= i;
                    break;
                }

                else if (i == str_len-1) {
                    return offset;
                }
            }
        }
    }

    return -1;
}


static void _print_data_value_from_csv(char *csv_filepath, int key)
{
    FILE *csv_fp = fopen(csv_filepath, "rb");
    char *result = csv_get_string_by_value(csv_fp, key);
    fclose(csv_fp);

    if (strcmp(result, "") == 0) return;

    printf("%s\n", result);
    free(result);
}


static bool _read_tiff_header(FILE *image_file)
{
    if ((byte_alignment_offset = _get_string_offset_from_start_of_file(image_file, "Exif")) == -1) {
        //fprintf(stderr, "[ERROR] could not find image header\n%s : %d\n", __FILE__, __LINE__);
        return false;
    }

    fseek(image_file, 2, SEEK_CUR);   // skip 2x 0x00
    if (fgetc(image_file) == 'M') {   // Motorola
        strcpy(byte_alignment, "MM");
    }
    else {
        strcpy(byte_alignment, "II"); // Intel
    }

    byte_alignment_offset += 2;       // account for the skipped null bytes

    fseek(image_file, 3, SEEK_CUR);   // skip the next I or M and 0x002A or 0x2A00
    next_ifd_offset = _read_n_byte_int(image_file, 4);
    return true;
}


// can be used with `data format` 5, 10; length is expected to be 8
static void _print_fract_value_by_offset(FILE *image_file, int offset)
{
    long old_file_pos = ftell(image_file);
    _goto_file_offset(image_file, offset);

    printf("%d/", _read_n_byte_int(image_file, 4));
    printf("%d\n", _read_n_byte_int(image_file, 4));

    fseek(image_file, old_file_pos, SEEK_SET);
}


static void _print_ascii_string_by_offset(FILE *image_file, int offset, int string_length)
{
    int old_file_pos = ftell(image_file);
    char letter;

    _goto_file_offset(image_file, offset);

    for (int i = 0; i < string_length; i++) {
        letter = fgetc(image_file);
        putchar(letter);
    }
    putchar('\n');

    fseek(image_file, old_file_pos, SEEK_SET);
}


static void _print_value_by_offset(FILE *image_file, int offset, int total_components_length, int n_components)
{
    long old_file_pos = ftell(image_file);
    _goto_file_offset(image_file, offset);

    int single_component_length = total_components_length/n_components;

    for (int i = 0; i < n_components; i++) {
        if (i == n_components - 1) {
            printf("%d\n", _read_n_byte_int(image_file, single_component_length));
            break;
        }
        printf("%d, ", _read_n_byte_int(image_file, single_component_length));
    }

    fseek(image_file, old_file_pos, SEEK_SET);
}


static int _get_total_components_length(int format_value, int components_count)
{
    switch (format_value) {
        case 1: case 2: case 6: case 7:
            return 1 * components_count;

        case 3: case 8:
            return 2 * components_count;

        case 4: case 9: case 11:
            return 4 * components_count;

        case 5: case 10: case 12:
            return 8 * components_count;

        default:
            fprintf(stderr, "\n[ERROR] Unrecognized format value (%d)\n%s : %d\n", format_value, __FILE__, __LINE__);
            exit(EXIT_FAILURE);
    }
}


static void _indent_result(size_t keyword_length)
{
    keyword_length++; // count in a colon

    if (keyword_length < 5)
        printf("\t\t\t\t\t");

    else if (keyword_length < 8)
        printf("\t\t\t\t");

    else if (keyword_length < 16)
        printf("\t\t\t");

    else if (keyword_length < 20)
        printf("\t\t");

    else if (keyword_length < 28)
        putchar('\t');
}


static bool _data_format_is_fractional(int data_format)
{
    if (data_format == 5 || data_format == 10) return true;
    return false;
}


static bool _data_format_is_int(int data_format)
{
    if (data_format == 1 || data_format == 3 || data_format == 4 || data_format == 6 || \
        data_format == 7 || data_format == 8 || data_format == 9) {
        return true;
    }
    return false;
}


static void _print_resolution_unit(FILE *image_file)
{
    int c, i;

    for (i = 0; i < 4; i++) {
        c = _read_n_byte_int(image_file, 1);

        if (c == 0)
            continue;

        if (c > 3)
            continue;

        break;
    }
    fseek(image_file, 3-i, SEEK_CUR);
    _print_data_value_from_csv(RESOLUTION_UNIT_TAGS_FILEPATH, c);
}


static void _print_ycbcr_positioning(FILE *image_file)
{
    int c, i;

    for (i = 0; i < 4; i++) {
        c = _read_n_byte_int(image_file, 1);

        if (c != 1 && c != 2)
            continue;
        break;
    }
    fseek(image_file, 3-i, SEEK_CUR);
    _print_data_value_from_csv(YCBCR_POSITIONING_TAGS_FILEPATH, c); 
}


static void _print_custom_rendered(FILE *image_file)
{
    int c, i;

    for (i = 0; i < 4; i++) {
        c = _read_n_byte_int(image_file, 1);

        if (c > 8 || c == 5)
            continue;

        break;
    }

    fseek(image_file, 3-i, SEEK_CUR);
    _print_data_value_from_csv(CUSTOM_RENDERED_TAGS_FILEPATH, c);
}


static void _print_exposure_mode(FILE *image_file)
{
    int c, i;

    for (i = 0; i < 4; i++) {
        c = _read_n_byte_int(image_file, 1);

        if (c == 0 || c == 1 || c == 2) {
            break;
        }
    }

    fseek(image_file, 3-i, SEEK_CUR);
    _print_data_value_from_csv(EXPOSURE_MODE_TAGS_FILEPATH, c);
}


static void _print_white_balance(FILE *image_file)
{
    // the possible results overlap,
    // so we can just call this function
    _print_exposure_mode(image_file);
}


static void _print_focal_length_in_35_mm(FILE *image_file)
{
    int c, i;

    for (i = 0; i < 4; i++) {
        c = _read_n_byte_int(image_file, 1);

        if (c == 0) {
            continue;
        }

        fseek(image_file, 3-i, SEEK_CUR);
        printf("%dmm\n", c);
        return;
    }
}


static void _print_scene_capture_type(FILE *image_file)
{
    int c, i;

    for (i = 0; i < 4; i++) {
        c = _read_n_byte_int(image_file, 1);

        if (c > 4) {
            continue;
        }

        break;
    }
    
    fseek(image_file, 3-i, SEEK_CUR);
    _print_data_value_from_csv(SCENE_CAPTURE_TAGS_FILEPATH, c);
}


static void _print_gain_control(FILE *image_file)
{
    int c, i;

    for (i = 0; i < 4; i++) {
        c = _read_n_byte_int(image_file, 1);

        if (c > 4) {
            continue;
        }
        break;
    }

    fseek(image_file, 3-i, SEEK_CUR);
    _print_data_value_from_csv(GAIN_CONTROL_TAGS_FILEPATH, c);
}


static void _print_contrast(FILE *image_file)
{
    int c, i;

    for (i = 0; i < 4; i++) {
        c = _read_n_byte_int(image_file, 1);

        if (c > 2) {
            continue;
        }
        break;
    }

    fseek(image_file, 3-i, SEEK_CUR);
    _print_data_value_from_csv(CONTRAST_TAGS_FILEPATH, c);
}


static void _print_saturation(FILE *image_file)
{
    // the possible outcomes are the same as with contrast,
    // so we can reause that function
    _print_contrast(image_file);
}


static void _print_sharpness(FILE *image_file)
{
    // also shares the same outcomes with the contrast tag
    _print_contrast(image_file);
}


static void _print_orientation(FILE *image_file)
{
    int value = _read_n_byte_int(image_file, 4);
    _print_data_value_from_csv(ORIENTATION_TAGS_FILEPATH, value);
}


static void _print_compression(FILE *image_file)
{
    int key = _read_4_byte_data_value(image_file);
    _print_data_value_from_csv(EXIF_COMPRESSION_TAGS_FILEPATH, key);
}


static bool _detect_tags_with_their_own_functions(FILE *image_file, int tag_number)
{
    if (tag_number == 0x0128) 
        _print_resolution_unit(image_file);

    else if (tag_number == 0x0213)
        _print_ycbcr_positioning(image_file);

    else if (tag_number == 0xa401)
        _print_custom_rendered(image_file);

    else if (tag_number == 0xa402)
        _print_exposure_mode(image_file);

    else if (tag_number == 0xa403)
        _print_white_balance(image_file);

    else if (tag_number == 0xa405)
        _print_focal_length_in_35_mm(image_file);

    else if (tag_number == 0xa406)
        _print_scene_capture_type(image_file);

    else if (tag_number == 0xa407)
        _print_gain_control(image_file);

    else if (tag_number == 0xa408)
        _print_contrast(image_file);

    else if (tag_number == 0xa409)
        _print_saturation(image_file);

    else if (tag_number == 0xa40a)
        _print_sharpness(image_file);

    else if (tag_number == 0x0112)
        _print_orientation(image_file);

    else if (tag_number == 0x0103)
        _print_compression(image_file);

    else return false;
    return true;
}


// expects that the file pointer is right before the ifd starts
// that is, before the number of directory entries
// returns the offset to the next IFD
static int _print_ifd_entry_data(FILE *image_file, FILE *csv_fp)
{
    static Exif_Tag_Array exif_tags = {0};
    int tag_number, data_format, n_components, data_offset, total_components_length;
    int n_entries = _read_n_byte_int(image_file, 2);

    for (int i = 0; i < n_entries; i++) {
        tag_number = _read_n_byte_int(image_file, 2);
        data_format = _read_n_byte_int(image_file, 2);
        n_components = _read_n_byte_int(image_file, 4);

        if (!_tag_has_occured(exif_tags, tag_number)) tags_append(exif_tags, tag_number);
        else {
            fseek(image_file, 4, SEEK_CUR);
            continue;
        }

        char *tag_string = csv_get_string_by_value(csv_fp, tag_number);

        if (strcmp(tag_string, "") == 0) {
            free(tag_string);
            continue;
        }

        printf("%s: ", tag_string);
        _indent_result(strlen(tag_string));
        free(tag_string);

        if (_detect_tags_with_their_own_functions(image_file, tag_number))
            continue;

        if ((total_components_length = _get_total_components_length(data_format, n_components)) > 4) {
            data_offset = _read_n_byte_int(image_file, 4);
        }
        else if (data_format == 2) {
            _print_ascii_string_by_offset(image_file, ftell(image_file)-byte_alignment_offset, 4);
            continue;
        }
        else {
            printf("%d\n", _read_4_byte_data_value(image_file));
            continue;
        }


        if (_data_format_is_fractional(data_format)) {
            _print_fract_value_by_offset(image_file, data_offset);
        }
        else if (_data_format_is_int(data_format) && total_components_length > 4) {
            _print_value_by_offset(image_file, data_offset, total_components_length, n_components);
        }
        else if (n_components > 4 && data_format == 2) {
            _print_ascii_string_by_offset(image_file, data_offset, n_components);
        }
    }

    next_ifd_offset = _read_4_byte_data_value(image_file);
    if (next_ifd_offset == 0) {
        free(exif_tags.tags);
        return 0;
    }

    _goto_file_offset(image_file, next_ifd_offset);
    _print_ifd_entry_data(image_file, csv_fp);

    return 0;
}


static bool _find_jfif_start(FILE *image_file)
{
    int c0, c1, c2, c3, c4;
    int identifier_byte_0 = 0x4a;
    int identifier_byte_1 = 0x46;
    int identifier_byte_2 = 0x49;
    int identifier_byte_3 = 0x46;
    int identifier_byte_4 = 0x00;

    fseek(image_file, 0, SEEK_SET);
    while ((c0 = fgetc(image_file)) != EOF) {
        if (c0 == identifier_byte_0) {
            c1 = fgetc(image_file);
            c2 = fgetc(image_file);
            c3 = fgetc(image_file);
            c4 = fgetc(image_file);

            if (c1 == identifier_byte_1 && c2 == identifier_byte_2 && c3 == identifier_byte_3 && c4 == identifier_byte_4) {
                jfif_is_resent = true;
                return true;
            }

            fseek(image_file, -4, SEEK_CUR);
        }
    }

    return false;
}


static int _get_shortened_marker_value(int marker)
{
    return marker - (0xf * _get_nth_power(HEX_MULTIPLIER, 2)) - (0xf * _get_nth_power(HEX_MULTIPLIER, 3));
}


static bool _find_segment_marker(FILE *image_file, int hex_marker)
{
    int short_marker = _get_shortened_marker_value(hex_marker);

    int c, prev;
    while ((c = fgetc(image_file)) != EOF) {
        if (c == short_marker) {
            fseek(image_file, -2, SEEK_CUR);
            prev = fgetc(image_file);
            fseek(image_file, 1, SEEK_CUR);

            if (prev == 0xFF) {
                return true;
            }
        }
    }
    return false;
}


static void _print_ffe0_segment_data(FILE *image_file)
{
    if (!_find_jfif_start(image_file)) return;

    int jfif_version_1 = _read_n_byte_int(image_file, 1);
    int jfif_version_2 = _read_n_byte_int(image_file, 1);
    int units = _read_n_byte_int(image_file, 1);
    int x_resolution = _read_2_byte_data_value(image_file);
    int y_resolution = _read_2_byte_data_value(image_file);
    int x_thumbnail = _read_n_byte_int(image_file, 1);
    int y_thumbnail = _read_n_byte_int(image_file, 1);

    if (jfif_version_2 >= 0x10) {
        printf("jfif version: %d.%d\n", jfif_version_1, jfif_version_2);
    }
    else {
        printf("jfif version: %d.0%d\n", jfif_version_1, jfif_version_2);
    }
    printf("units: %d\n", units);

    if (jfif_is_resent) return;
    printf("x resolution: %d\n", x_resolution);
    printf("y resolution: %d\n", y_resolution);

    if (x_thumbnail == 0) return;
    printf("x thumbnail: %d\n", x_thumbnail);
    printf("y thumbnail: %d\n", y_thumbnail);
}


static void _print_fffe_segment_data(FILE *image_file)
{
    int count = 0;

    while (_find_segment_marker(image_file, 0xFFFE)) {
        count++;
        int length = _read_2_byte_data_value(image_file) - 2;
        int old_file_pos = ftell(image_file);

        printf("comment [%d]: ", count);
        _print_ascii_string_by_offset(image_file, old_file_pos, length);
        fseek(image_file, length, SEEK_CUR);
    }
}


void jpg_print_segment_markers_data(FILE *image_file)
{
    fseek(image_file, 0, SEEK_SET);
    _print_ffe0_segment_data(image_file); // jfif

    fseek(image_file, 0, SEEK_SET);
    _print_fffe_segment_data(image_file); // comments
    return;
}


void jpg_print_exif_data(FILE *image_file)
{
    bool file_has_tiff_header = _read_tiff_header(image_file);

    printf("byte alignment:\t\t\t%s", byte_alignment);
    strcmp(byte_alignment, "MM") == 0 ? printf(" (Big Endian/Motorola)\n") : printf(" (Little Endian/Intel)\n");
    if (!file_has_tiff_header) return;

    FILE *csv_fp = fopen(JPG_EXIF_TAGS_FILEPATH, "rb");
    _print_ifd_entry_data(image_file, csv_fp);
    fclose(csv_fp);
}

