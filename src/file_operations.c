#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include "file_operations.h"

#ifdef _WIN32
#   pragma warning(disable: 4996)
#endif

#define SIGNATURE_END_INDEX 8
#define IHDR_LENGTH 13
#define DATE_LENGTH 26
#define HEX_MULTIPLIER 256


// index2 is non-inclusive
char *_slice_string(const char *str, int index1, int index2) {
    int new_str_len = index2 - index1;

    char *new_str = malloc(new_str_len*sizeof(char));
    int new_str_index = 0;
    int str_index = index1;

    for (; new_str_index <= new_str_len; ) {
        new_str[new_str_index++] = str[str_index++];
    }

    return new_str;
}


char *get_filename(const char *path) {
    int path_len = strlen(path);
    int last_slash_index;
    char current, *filename;

    for (int i = 0; i < path_len; i++) { 
        current = path[i];

        if (current == '\\' || current == '/') {
            last_slash_index = i;
        }
    }
    filename = _slice_string(path, last_slash_index+1, path_len);

    return filename;
}


char *get_filetype_extension(const char *filename) {
    int filename_len = (int) strlen(filename);
    int extension_start_index;

    for (int i = 0; i < filename_len; i++) {
        if (filename[i] == '.') {
            extension_start_index = i + 1;
        }
    }

    char *extension = _slice_string(filename, extension_start_index, filename_len);

    if (strcmp(extension, "png") != 0 && strcmp(extension, "PNG") != 0) {
        fprintf(stderr, "\n[ERROR] The filetype has to be PNG, not %s.\n", extension);
        exit(EXIT_FAILURE);
    }

    return extension;
}


int get_file_size(const char *path) {
    struct stat file_status;
    if (stat(path, &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}


char *get_last_change_date(const char *path) {
    struct stat file_status;
    if (stat(path, &file_status) < 0) {
        return "";
    }

    time_t time_t_date = file_status.st_mtime;
    char *date = malloc(DATE_LENGTH * sizeof(char));
    date = ctime(&time_t_date);

    return date;
}


bool _find_chunk(FILE *image_file, char name_char_0, char name_char_1, char name_char_2, char name_char_3) {
    /* attempts to find a 4-character chunk name, stops at the last character of the name */
    int c0, c1, c2, c3;

    while ((c0 = fgetc(image_file)) != EOF) {
        if (c0 == name_char_0) {
            c1 = fgetc(image_file);
            c2 = fgetc(image_file);
            c3 = fgetc(image_file);

            if (c1 == name_char_1 && c2 == name_char_2 && c3 == name_char_3) {
                return true;
            }
            else {
                fseek(image_file, -3, SEEK_CUR);
            }
        }
    }

    return false;
}


int _get_4_byte_int(FILE *image_file) {
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


int _get_index_of_string_in_array(char *str_array[], int array_length, const char *str) {
    for (int i = 0; i < array_length; i++) {
        if (strcmp(str_array[i], str) == 0) {
            return i;
        }
    } 
    return -1;
}


bool _string_is_present_in_file(FILE *image_file, char *str) {
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


void _indent_keyword_value(size_t keyword_length) {
    keyword_length++; // count in the color after the keyword

    if (keyword_length < 7)
        printf("\t\t\t\t\t");

    else if (keyword_length < 11)
        printf("\t\t\t\t");

    else if (keyword_length < 16)
        printf("\t\t\t");

    else if (keyword_length < 19)
        printf("\t\t");
}


/* **********************
 *
 *  IHDR (header) chunk
 *
 * *********************/

int get_print_IHDR_chunk_data(FILE *image_file) {
    int width, height, bit_depth, color_type, compression_method, filter_method, interlace_method;
    int width_byte_length = 4;
    int height_byte_length = 4;

    // skip png signature bytes + 4 length bytes + 4 chunk type (IHDR) bytes 
    fseek(image_file, SIGNATURE_END_INDEX+8, SEEK_CUR);

    width = 0;
    for (int i = 0; i < width_byte_length; i++) {
        if (width == 0) {
            width = fgetc(image_file);
        } else {
            width = width * HEX_MULTIPLIER + fgetc(image_file);
        }
    }

    height = 0;
    for (int i = 0; i < height_byte_length; i++) {
        if (height == 0) {
            height = fgetc(image_file);
        } else {
            height = height * HEX_MULTIPLIER + fgetc(image_file);
        }
    }

    bit_depth = fgetc(image_file);
    color_type = fgetc(image_file);
    compression_method = fgetc(image_file);
    filter_method = fgetc(image_file);
    interlace_method = fgetc(image_file);

    printf("\n\n ------------ IHDR chunk data ------------ \n");
    printf("width:\t\t\t\t%d\n", width);
    printf("height:\t\t\t\t%d\n", height);
    printf("bit depth:\t\t\t%d\n", bit_depth);
    printf("color type:\t\t\t%d\n", color_type);
    printf("compression method:\t\t%d\n", compression_method);
    printf("filter method:\t\t\t%d\n", filter_method);
    printf("interlace method:\t\t%d\n", interlace_method);

    return color_type;
}



/* ************************
 *
 *   PLTE (palette) chunk
 *
 * ************************/

void _get_PLTE_data(FILE *image_file) {
    int length = 0;

    // 4 bytes before "PLTE" are its length
    //     => -4 (for each letter in PLTE) + 4 for the length bytes
    fseek(image_file, -8, SEEK_CUR);

    length = _get_4_byte_int(image_file);

    printf("palette length:\t\t\t%d\n", length);
    printf("number of colors:\t\t%d\n", length/3);
}


void print_PLTE_chunk_data(FILE *image_file) {
    fseek(image_file, SIGNATURE_END_INDEX+IHDR_LENGTH, SEEK_SET);

    if (!_find_chunk(image_file, 'P', 'L', 'T', 'E')) return;

    printf("\n\n\n ------------ PLTE chunk data ------------\n");
    _get_PLTE_data(image_file);
}



/* *********************
 *
 *      tEXt chunk
 *
 * *********************/

void _print_text_element(FILE *image_file, int length) {
    int c;

    for (int i = 0; i < length; i++) {
        c = fgetc(image_file);
        putchar(c);
    }
    putchar('\n');
}


void _parse_tEXt_chunk(FILE *image_file) {
    int c, length;
    int iteration = 0;

    fseek(image_file, -5, SEEK_CUR);
    length = fgetc(image_file);
    fseek(image_file, 4, SEEK_CUR);

    while ((c = fgetc(image_file)) != 0) {
        iteration++;
        putchar(c);
    }
    printf("\t\t");

    length -= iteration+1;
    _print_text_element(image_file, length);
}


void print_tEXt_chunk_data(FILE *image_file) {
    static bool title_printed = false;

    /* tEXt has no ordering constraints,
    better look for it from the start of the file */

    if (!title_printed) {
        fseek(image_file, SIGNATURE_END_INDEX+IHDR_LENGTH, SEEK_SET);
    }

    if (!_find_chunk(image_file, 't', 'E', 'X', 't')) return;

    if (!title_printed) {
        printf("\n\n\n ------------ tEXt chunk data ------------\n");
        title_printed = true;
    }

    _parse_tEXt_chunk(image_file);
    print_tEXt_chunk_data(image_file);
}



/* *********************
 *
 *      iTXt chunk
 *
 * *********************/

void _print_iTXt_keyword_value(FILE *image_file, int keyword_length) {
    int c;

    fseek(image_file, keyword_length, SEEK_CUR);

    while ((c = fgetc(image_file)) != '<')
        putchar(c);
    putchar('\n');

    while ((c = fgetc(image_file)) != '\n') {
        ;
    }
}


void _detect_individual_iTXt_metadata_keyword(FILE *image_file, char **keywords, size_t keywords_arr_length) {
    int keyword_length;
    size_t i;
    int c = fgetc(image_file);
    bool word_found = false;

    for (i = 0; i < keywords_arr_length; i++) {
        if (*keywords[i] != c)
            continue;

        keyword_length = strlen(keywords[i]);

        for (int j = 1; j < keyword_length; j++) {
            if (!(keywords[i][j] == fgetc(image_file))) {
                fseek(image_file, -j, SEEK_CUR);
                break;
            }
            else if (j == keyword_length-1) {
                word_found = true;
                break;
            }
        }

        if (word_found) break;
    }

    fseek(image_file, -keyword_length+1, SEEK_CUR);
    printf("%s:", keywords[i]);

    _indent_keyword_value(keyword_length); // tab before printing value
    _print_iTXt_keyword_value(image_file, keyword_length);
    return;
}


void _find_iTXt_metadata(FILE *image_file, size_t chunk_length, char **keywords, size_t keywords_length, char *metadata_name) {
    // re-allign file pointer
    fseek(image_file, SIGNATURE_END_INDEX+IHDR_LENGTH, SEEK_SET);
    _find_chunk(image_file, 'i', 'T', 'X', 't');

    int c, c1;
    int metadata_name_length = strlen(metadata_name);

    for (size_t i = 0; i < chunk_length; i++) {
        c = fgetc(image_file);

        if (c == metadata_name[0]) {
            for (int name_index = 1; name_index < metadata_name_length; name_index++) {
                c1 = fgetc(image_file);

                if (c1 != metadata_name[name_index]) {
                    fseek(image_file, -name_index, SEEK_CUR);
                    break;
                }
                else if (name_index == metadata_name_length-1) {
                    fseek(image_file, 1, SEEK_CUR); // skip ':'
                    _detect_individual_iTXt_metadata_keyword(image_file, keywords, keywords_length);
                    break;
                }
            }
        }
    }
}


void print_iTXt_chunk_data(FILE *image_file) {

    fseek(image_file, SIGNATURE_END_INDEX+IHDR_LENGTH, SEEK_SET);

    if (!_find_chunk(image_file, 'i', 'T', 'X', 't')) return;

    
    printf("\n\n\n ------------ iTXt chunk data ------------\n");
    
    fseek(image_file, -8, SEEK_CUR);
    size_t length = (unsigned long long) _get_4_byte_int(image_file);
    fseek(image_file, 4, SEEK_CUR);

    if (_string_is_present_in_file(image_file, "tiff")) {
        size_t keywords_length = 9;

        char *tiff_keywords[] = {
            "ResolutionUnit",
            "XResolution",
            "YResolution",
            "Orientation",
            "ImageWidth",
            "ImageLength",
            "BitsPerSample",
            "PhotometricInterpretation",
            "SamplesPerPixel"
        };
        _find_iTXt_metadata(image_file, length, tiff_keywords, keywords_length, "<tiff");
    }
}



/* *********************
 *
 *      bKGD chunk
 *
 * *********************/

void _display_bKGD_color(FILE *image_file, int color_type) {
    int byte0, byte1, byte2;

    switch (color_type) {
        // case 3:         // indexed color
        
        case 0: case 4: // grayscale with/without alpha => 2 bytes
            fseek(image_file, 1, SEEK_CUR);
            byte0 = fgetc(image_file);
            printf("color value:\t\t\t%d\n", byte0);
            break;
        
        case 2: case 6: // truecolor, with/without alpha => 3 * 2 bytes
            fseek(image_file, 1, SEEK_CUR);
            byte0 = fgetc(image_file);
            fseek(image_file, 1, SEEK_CUR);
            byte1 = fgetc(image_file);
            fseek(image_file, 1, SEEK_CUR);
            byte2 = fgetc(image_file);

            printf("background color value:\t\t[%d, %d, %d]\n", byte0, byte1, byte2);
            break;
    }
}


void print_bKGD_chunk_data(FILE *image_file, int color_type) {
    fseek(image_file, IHDR_LENGTH, SEEK_SET);

    if (!_find_chunk(image_file, 'b', 'K', 'G', 'D')) return;

    printf("\n\n\n ------------ bKGD chunk data ------------\n");
    _display_bKGD_color(image_file, color_type);
}



 /* *********************
 *
 *      cHRM chunk
 *    (chromaticities)
 *
 * *********************/

void print_cHRM_chunk_data(FILE *image_file) {
    // calling this fn right after IHDR, no need for resetting file pointer

    if (!_find_chunk(image_file, 'c', 'H', 'R', 'M')) return;

    /* each stored in 4 bytes
    0 -> while point x
    1 -> while point y
    2 -> red x
    3 -> red y
    4 -> green x
    5 -> green y
    6 -> blue x
    7 -> blue y
    */
    int NUM_ITEMS = 8;
    int chromaticities[NUM_ITEMS];

    for (int i = 0; i < NUM_ITEMS; i++) {
        chromaticities[i] = _get_4_byte_int(image_file);
    }

    printf("\n\n\n ------------ cHRM chunk data ------------\n");

    printf("while point x:\t\t\t%d\n", chromaticities[0]);
    printf("while point y:\t\t\t%d\n", chromaticities[1]);
    printf("red x:\t\t\t\t%d\n", chromaticities[2]);
    printf("red y:\t\t\t\t%d\n", chromaticities[3]);
    printf("green x:\t\t\t%d\n", chromaticities[4]);
    printf("green y:\t\t\t%d\n", chromaticities[5]);
    printf("blue x:\t\t\t\t%d\n", chromaticities[6]);
    printf("blue y:\t\t\t\t%d\n", chromaticities[7]);
}



 /* *********************
 *
 *      sBIT chunk
 *
 * *********************/

void print_sBIT_chunk_data(FILE *image_file, int color_type) {
    fseek(image_file, SIGNATURE_END_INDEX+IHDR_LENGTH, SEEK_SET); 
    if (!_find_chunk(image_file, 's', 'B', 'I', 'T')) return;

    printf("\n\n\n ------------ sBIT chunk data ------------\n");

    switch (color_type) {
        case 0:
            printf("significant bits count:\t");
            printf("%d\n", fgetc(image_file));
            break;

        case 2:
            printf("(number of significant bits to these channels)\n");
            printf("red:\t\t\t\t%d\n", fgetc(image_file));
            printf("green:\t\t\t\t%d\n", fgetc(image_file));
            printf("blue:\t\t\t\t%d\n", fgetc(image_file));

            break;
        
        case 3:
            printf("(number of bits significant to these components)\n");
            printf("red:\t\t\t\t%d\n", fgetc(image_file));
            printf("green:\t\t\t\t%d\n", fgetc(image_file));
            printf("blue:\t\t\t\t%d\n", fgetc(image_file));
            break;

        case 4:
            printf("(number of bits significant in the source data)\n");
            printf("grayscale:\t\t\t%d\n", fgetc(image_file));
            printf("alpha:\t\t\t\t%d\n", fgetc(image_file));
            break;

        case 6:
            printf("(number of significant bits to these channels)\n");
            printf("red:\t\t\t\t%d\n", fgetc(image_file));
            printf("green:\t\t\t\t%d\n", fgetc(image_file));
            printf("blue:\t\t\t\t%d\n", fgetc(image_file));
            printf("alpha:\t\t\t\t%d\n", fgetc(image_file));
            break;
    }
}



 /* *********************
 *
 *      tRNS chunk
 *
 * *********************/

void print_tRNS_chunk_data(FILE *image_file, int color_type) {
    fseek(image_file, SIGNATURE_END_INDEX+IHDR_LENGTH, SEEK_SET);
    if (!_find_chunk(image_file, 't', 'R', 'N', 'S')) return;

    int c0, c1;
    printf("\n\n\n ------------ tRNS chunk data ------------\n");

    switch (color_type) {
        case 0: // grayscale
            c0 = fgetc(image_file);
            c1 = fgetc(image_file);
            printf("gray level value:\t\t%d\n", c1 + c0 * HEX_MULTIPLIER);
            break;

        case 2: // truecolor 
            for (int i = 0; i < 3; i++) {
                c0 = fgetc(image_file);
                c1 = fgetc(image_file);

                if (i == 0)
                    printf("red value:\t\t\t%d\n", c1 + c0 * HEX_MULTIPLIER);

                else if (i == 1)
                    printf("green value:\t\t\t%d\n", c1 + c0 * HEX_MULTIPLIER);

                else if (i == 2)
                    printf("blue value:\t\t\t%d\n", c1 + c0 * HEX_MULTIPLIER);
            }
            break;

        case 3: // indexed color
            printf("chunk contains alpha values correspondingto each entry in the PLTE chunk\n");
            break;
    }
}



 /* ********************
 *
 *      sRGB chunk
 *
 * ********************/

void print_sRGB_chunk_data(FILE *image_file) {
    fseek(image_file, SIGNATURE_END_INDEX+IHDR_LENGTH, SEEK_SET);
    if (!_find_chunk(image_file, 's', 'R', 'G', 'B')) return;

    printf("\n\n\n ------------ sRGB chunk data ------------\n");
    printf("rendering intent: ");

    int c = fgetc(image_file);

    switch (c) {
        case 0:
            printf("perceptual");
            return;

        case 1:
            printf("relative colorimetric");
            return;

        case 2:
            printf("saturation");
            return;

        case 3:
            printf("absolute colorimetric");
            return;
    }
}



 /* ********************
 *
 *      eXIf chunk
 *
 * ********************/

void print_eXIf_chunk_data(FILE *image_file) {
    fseek(image_file, SIGNATURE_END_INDEX+IHDR_LENGTH, SEEK_SET);

    if (!_find_chunk(image_file, 'e', 'X', 'I', 'f')) return;
    int i, c, length;
    bool last_char_newline = true;
    bool brace_found = false;

    printf("\n\n\n ------------ eXIf chunk data ------------\n");
    fseek(image_file, -8, SEEK_CUR);

    length = _get_4_byte_int(image_file);

    // first find the '{' character
    for (i = 0; i < length; i++) {
        c = fgetc(image_file);

        if (c == '{') {
            brace_found = true;
            break;
        }
    }

    if (!brace_found) {
        printf("couldn't decode eXIf chunk data\n");
        return;
    }

    bool sub_array = false;

    for (; i < length+1; i++) {
        c = fgetc(image_file);

        if (c == '{') {
            sub_array = true;
        }

        if (c == '}' && sub_array == true) {
            sub_array = false;
        }

        if (c == ',' && sub_array == false) {
            putchar('\n');
            last_char_newline = true;
            continue;
        }

        if (c == ':' && fgetc(image_file) != ' ') {
            printf(": ");
            last_char_newline = false;
            fseek(image_file, -1, SEEK_CUR);

            continue;
        }

        if (c == '"') {
            if (last_char_newline)
                continue;

            putchar(' ');
            last_char_newline = false;
            continue;
        }

        putchar(c);
        last_char_newline = false;
    }
}



 /* ***********************
 *
 *  common private chunks
 *
 * ***********************/

void search_for_common_private_chunks(FILE *image_file) {
    char *private_chunks_names[] = {
        "prVW",
        "mkBF",
        "mkBS",
        "mkBT",
        "mkTS"
    };

    int names_array_length = sizeof(private_chunks_names)/sizeof(char *);
    int c0, c1, c2, c3;
    bool title_printed = false;

    for (int i = 0; i < names_array_length; i++) {
        fseek(image_file, SIGNATURE_END_INDEX+IHDR_LENGTH, SEEK_SET);

        c0 = private_chunks_names[i][0];
        c1 = private_chunks_names[i][1];
        c2 = private_chunks_names[i][2];
        c3 = private_chunks_names[i][3];

        if (_find_chunk(image_file, c0, c1, c2, c3)) {
            if (!title_printed) {
                printf("\n\n\n ------------ private chunks ------------\n");
                title_printed = true;
            }

            // check if there are more occurrences
            if (_find_chunk(image_file, c0, c1, c2, c3)) {
                printf("[%s] - multiple private chunks of type %s were found\n", private_chunks_names[i], private_chunks_names[i]);
                continue;
            }

            printf("[%s] - one chunk of type %s was found\n", private_chunks_names[i], private_chunks_names[i]);
        }
    }
}


 /* *********************
 *
 *  other small chunks
 *  (gAMA, pHYs, tIME)
 *
 * *********************/

void print_gAMA_chunk_data(FILE *image_file) {
    fseek(image_file, SIGNATURE_END_INDEX+IHDR_LENGTH, SEEK_SET);
    
    if (!_find_chunk(image_file, 'g', 'A', 'M', 'A')) return;

    printf("\n\n\n ------------ gAMA chunk data ------------\n");
    printf("gamma value:\t\t\t");
    printf("%d\n", _get_4_byte_int(image_file));
}


void print_pHYs_chunk_data(FILE *image_file) {
    fseek(image_file, SIGNATURE_END_INDEX+IHDR_LENGTH, SEEK_SET);
    if (!_find_chunk(image_file, 'p', 'H', 'Y', 's')) return;

    int x_axis_pixels, y_axis_pixels, unit_specifier;

    printf("\n\n\n ------------ pHYs chunk data ------------\n");
    x_axis_pixels = _get_4_byte_int(image_file);
    y_axis_pixels = _get_4_byte_int(image_file);
    unit_specifier = fgetc(image_file);

    printf("unit:\t\t\t\t%s\n", unit_specifier == 0 ? "unknown (pixel aspect ratio only)" : "meter (pixels per unit)");
    printf("x axis:\t\t\t\t%d\n", x_axis_pixels);
    printf("y axis:\t\t\t\t%d\n", y_axis_pixels);
}


void print_tIME_chunk_data(FILE *image_file) {
    fseek(image_file, SIGNATURE_END_INDEX+IHDR_LENGTH, SEEK_SET);
    if (!_find_chunk(image_file, 't', 'I', 'M', 'E')) return;

    int year, month, day, hour, minute, second;

    year = fgetc(image_file) * HEX_MULTIPLIER;
    year += fgetc(image_file);

    month = fgetc(image_file);
    day = fgetc(image_file);
    hour = fgetc(image_file);
    minute = fgetc(image_file);
    second = fgetc(image_file);

    printf("\n\n\n ------------ tIME chunk data ------------\n");
    printf("last data modification date: %d/%d/%d/%d/%d/%d (Y/M/D/h/m/s)\n", year, month, day, hour, minute, second);
}

