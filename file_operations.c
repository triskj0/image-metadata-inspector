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

    if (strcmp(extension, "png") != 0) {
        printf("\n[ERROR] The filetype has to be PNG, not %s.\n", extension);
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

    return date;
}



/* **********************
 *
 *  IHDR (header) chunk
 *
 * *********************/


void print_IHDR_chunk_data(const char *path, FILE *image_file) {
    int c;
    int index = 0;
    int length, width, height, bit_depth, color_type, compression_method, filter_method, interlace_method;

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
    printf("width:\t\t\t%d\n", width);
    printf("height:\t\t\t%d\n", height);
    printf("bit depth:\t\t%d\n", bit_depth);
    printf("color type:\t\t%d\n", color_type);
    printf("compression method:\t%d\n", compression_method);
    printf("filter method:\t\t%d\n", filter_method);
    printf("interlace method:\t%d\n\n", interlace_method);
}



/* ************************
 *
 *   PLTE (palette) chunk
 *
 * ************************/

void _get_PLTE_data(FILE *image_file) {
    int c, red_value, green_value, blue_value;
    int length = 0;
    int number_of_color_examples;

    // 4 bytes before "PLTE" are its length
    fseek(image_file, -8, SEEK_CUR);

    for (int i = 4; i > 0; i --) {
        c = fgetc(image_file);

        switch (i) {
            case 4:
                length += c * 256 * 256 * 256;
                break;

            case 3:
                length += c * 256 * 256;
                break;

            case 2:
                length += c * 256;
                break;

            case 1:
                length += c;
        }
    }

    printf("\npalette length:\t\t%d", length);
    printf("\nnumber of colors:\t%d", length/3);
}


void print_PLTE_chunk_data(FILE *image_file) {
    int c, next_c0, next_c1, next_c2;
    int bytes_moved = 0;

    while ((c = fgetc(image_file)) != EOF) {
        if (c == 'I') { // must be before IDAT
            next_c0 = fgetc(image_file);
            next_c1 = fgetc(image_file);
            next_c2 = fgetc(image_file);

            if (next_c0 == 'D' && next_c1 == 'A' && next_c2 == 'T') {
                break;
            } else {
                fseek(image_file, 3, SEEK_CUR);
                continue;
            }
        }

        if (c == 'P') {
            next_c0 = fgetc(image_file);
            next_c1 = fgetc(image_file);
            next_c2 = fgetc(image_file);

            if (next_c0 == 'L' && next_c1 == 'T' && next_c2 == 'E') {
                printf("\n\n ------------ PLTE chunk data ------------ ");
                _get_PLTE_data(image_file);
                return;

            } else {
                fseek(image_file, 3, SEEK_CUR);
            }
        }
    }
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

    int first_char, c, c2, c3, length;

    fseek(image_file, -5, SEEK_CUR); // the byte before "tEXt" stores its length
    length = fgetc(image_file);

    fseek(image_file, 4, SEEK_CUR);
    first_char = fgetc(image_file);


    switch (first_char) {
        case 'T':
            printf("\ntitle:\t\t\t");

            // go forward -number of remaining characters in the keyword- + 1 for a null character
            fseek(image_file, 5, SEEK_CUR); 
            length -= 6; // length of "Title" + 1 for a null character

            _print_text_element(image_file, length);
            break;

        case 'A':
            printf("\nauthor:\t\t\t");
            fseek(image_file, 6, SEEK_CUR);
            length -= 7;

            _print_text_element(image_file, length);
            break;

        case 'D':
            if ((c2 = fgetc(image_file)) == 'e') { 
                printf("\ndescription:\t\t");
                fseek(image_file, 10, SEEK_CUR);
                length -= 12;

                _print_text_element(image_file, length);
                break;
            }

            printf("\ndisclaimer:\t\t");
            fseek(image_file, 9, SEEK_CUR);
            length -= 11;

            _print_text_element(image_file, length);
            break;

        case 'C':
            c2 = fgetc(image_file);

            if (c2 == 'r') {
                printf("\ncreation time:\t\t");
                fseek(image_file, 11, SEEK_CUR);
                length -= 13;

                _print_text_element(image_file, length);
                break;
            }

            c3 = fgetc(image_file);

            if (c3 == 'p') {
                printf("\ncopyright:\t\t");
                fseek(image_file, 7, SEEK_CUR);
                length -= 10;

                _print_text_element(image_file, length);
                break;
            }

            printf("\ncomment:\t\t");
            fseek(image_file, 5, SEEK_CUR);
            length -= 8;

            _print_text_element(image_file, length);
            break;

        case 'S':
            fseek(image_file, 1, SEEK_CUR);
            c3 = fgetc(image_file);
            
            if (c3 == 'u') {
                printf("\nsource:\t\t\t");
                fseek(image_file, 4, SEEK_CUR);
                length -= 7;

                _print_text_element(image_file, length);
                break;
            }

            printf("\nsoftware:\t\t");
            fseek(image_file, 6, SEEK_CUR);
            length -= 9;

            _print_text_element(image_file, length);
            break;


        case 'W':
            printf("\nwarning:\t\t\t");
            fseek(image_file, 7, SEEK_CUR);
            length -= 8;

            _print_text_element(image_file, length);
            break;
    }
}


void print_tEXt_chunk_data(FILE *image_file) {
    int c, next_c0, next_c1, next_c2;
    static bool title_printed = false;

    /* tEXt has no ordering constraints,
    so it's better to look for it from the start of the file */
    if (!title_printed)
        fseek(image_file, SIGNATURE_END_INDEX, SEEK_SET);

    while ((c = fgetc(image_file)) != EOF) {
        if (c == 't') {
            next_c0 = fgetc(image_file);
            next_c1 = fgetc(image_file);
            next_c2 = fgetc(image_file);

            if (next_c0 == 'E' && next_c1 == 'X' && next_c2 == 't') {
                if (!title_printed) {
                    printf("\n\n ------------ tEXt chunk data ------------ ");
                    title_printed = true;
                }
                _parse_tEXt_chunk(image_file);
                print_tEXt_chunk_data(image_file);
                break;
            }
            fseek(image_file, -3, SEEK_CUR);
        }
    }
}


void print_iTXt_chunk_data(FILE *image_file) {
    int c, next_c0, next_c1, next_c2, length; 
    static bool title_printed = false;

    /* no ordering constraints here either */
    if (!title_printed)
        fseek(image_file, SIGNATURE_END_INDEX, SEEK_SET);
    
    while ((c = fgetc(image_file)) != EOF) {
        if (c == 'i') {
            next_c0 = fgetc(image_file);
            next_c1 = fgetc(image_file);
            next_c2 = fgetc(image_file);

            if (next_c0 == 'T' && next_c1 == 'X' && next_c2 == 't') {
                if (!title_printed) {
                    printf("\n\n\n\n ------------ iTXt chunk data ------------ ");
                    title_printed = true;
                }
                
                fseek(image_file, -5, SEEK_CUR);
                length = fgetc(image_file);
                fseek(image_file, 5, SEEK_CUR);

                printf("\niTXt contents:\t\t");
                _print_text_element(image_file, length);
                break;
            }

            fseek(image_file, -3, SEEK_CUR);
        }
    }
}

