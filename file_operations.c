#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include "file_operations.h"


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
    char *date = malloc(26 * sizeof(char));
    errno_t err = ctime_s(date, 26*sizeof(char), &time_t_date);

    if (err) {
        fprintf(stderr, "[ERROR] an error occured while executing the ctime_s function.");
        exit(EXIT_FAILURE);
    }    

    return date;
}


bool _find_chunk(FILE *image_file, char name_char_0, char name_char_1, char name_char_2, char name_char_3) {
    /* attempts to find a 4-character chunk name, stops at the last character of the name */
    char c0, c1, c2, c3;

    while ((c0 = fgetc(image_file)) != EOF) {
        if (c0 == name_char_0) {
            c1 = fgetc(image_file);
            c2 = fgetc(image_file);
            c3 = fgetc(image_file);

            if (c1 == name_char_1 && c2 == name_char_2 && c3 == name_char_3) {
                return true;
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
                total += c * 256 * 256 * 256;
                break;

            case 3:
                total += c * 256 * 256;
                break;

            case 2:
                total += c * 256;
                break;

            case 1:
                total += c;
        }
    }
    return total;
}



/* **********************
 *
 *  IHDR (header) chunk
 *
 * *********************/

int get_print_IHDR_chunk_data(FILE *image_file) {
    int width, height, bit_depth, color_type, compression_method, filter_method, interlace_method;

    // skip png signature bytes + 4 length bytes + 4 chunk type (IHDR) bytes 
    fseek(image_file, SIGNATURE_END_INDEX+8, SEEK_CUR);

    width = 0;
    for (int i = 0; i < 4; i++) {
        if (width == 0) {
            width = fgetc(image_file);
        } else {
            width = width * 256 + fgetc(image_file);
        }
    }

    height = 0;
    for (int i = 0; i < 4; i++) {
        if (height == 0) {
            height = fgetc(image_file);
        } else {
            height = height * 256 + fgetc(image_file);
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

    /*
     * all available keywords:
     *      Title,
     *      Author,
     *      Description, Disclaimer,
     *      Copyright, CreationTime, Comment,
     *      Software, Source,
     *      Warning 
     */ 

    int first_char, c2, c3, length;

    fseek(image_file, -5, SEEK_CUR); // the byte before "tEXt" stores its length
    length = fgetc(image_file);

    fseek(image_file, 4, SEEK_CUR);
    first_char = fgetc(image_file);


    switch (first_char) {
        case 'T':
            printf("title:\t\t\t\t");

            // go forward -number of remaining characters in the keyword- + 1 for a null character
            fseek(image_file, 5, SEEK_CUR); 
            length -= 6; // length of "Title" + 1 for a null character

            _print_text_element(image_file, length);
            break;

        case 'A':
            printf("author:\t\t\t\t");
            fseek(image_file, 6, SEEK_CUR);
            length -= 7;

            _print_text_element(image_file, length);
            break;

        case 'D':
            if ((c2 = fgetc(image_file)) == 'e') { 
                printf("description:\t\t\t");
                fseek(image_file, 10, SEEK_CUR);
                length -= 12;

                _print_text_element(image_file, length);
                break;
            }

            printf("disclaimer:\t\t\t");
            fseek(image_file, 9, SEEK_CUR);
            length -= 11;

            _print_text_element(image_file, length);
            break;

        case 'C':
            c2 = fgetc(image_file);

            if (c2 == 'r') {
                printf("creation time:\t\t\t");
                fseek(image_file, 11, SEEK_CUR);
                length -= 13;

                _print_text_element(image_file, length);
                break;
            }

            c3 = fgetc(image_file);

            if (c3 == 'p') {
                printf("copyright:\t\t\t");
                fseek(image_file, 7, SEEK_CUR);
                length -= 10;

                _print_text_element(image_file, length);
                break;
            }

            printf("comment:\t\t\t");
            fseek(image_file, 5, SEEK_CUR);
            length -= 8;

            _print_text_element(image_file, length);
            break;

        case 'S':
            fseek(image_file, 1, SEEK_CUR);
            c3 = fgetc(image_file);
            
            if (c3 == 'u') {
                printf("source:\t\t\t\t");
                fseek(image_file, 4, SEEK_CUR);
                length -= 7;

                _print_text_element(image_file, length);
                break;
            }

            printf("software:\t\t\t");
            fseek(image_file, 6, SEEK_CUR);
            length -= 9;

            _print_text_element(image_file, length);
            break;


        case 'W':
            printf("warning:\t\t\t");
            fseek(image_file, 7, SEEK_CUR);
            length -= 8;

            _print_text_element(image_file, length);
            break;
    }
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

void print_iTXt_chunk_data(FILE *image_file) {
    static bool title_printed = false;

    /* same as with tEXt, no ordering constraints */

    if (!title_printed) {
        fseek(image_file, SIGNATURE_END_INDEX+IHDR_LENGTH, SEEK_SET);
    }

    if (!_find_chunk(image_file, 'i', 'T', 'X', 't')) return;
    
    if (!title_printed) {
        printf("\n\n\n ------------ iTXt chunk data ------------\n");
        title_printed = true;
    }
    
    fseek(image_file, -5, SEEK_CUR);
    int length = fgetc(image_file);
    fseek(image_file, 4, SEEK_CUR);

    printf("iTXt data:\t\t");
    _print_text_element(image_file, length);
    print_iTXt_chunk_data(image_file);
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
 *  other small chunks
 *  (gAMA, pHYs)
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

    printf("unit:\t\t\t\t%s\n", unit_specifier == 0 ? "unknown (pixel aspect ration only)" : "meter");
    printf("x axis:\t\t\t\t%d\n", x_axis_pixels);
    printf("y axis:\t\t\t\t%d\n", y_axis_pixels);
}


