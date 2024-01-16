#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include "file_operations.h"


// index2 is non-inclusive
char *slice_string(const char *str, int index1, int index2) {
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
    filename = slice_string(path, last_slash_index+1, path_len);

    return filename;
}


char *get_filetype_extension(const char *filename) {
    int filename_len = (int) strlen(filename);
    int extension_start_index;
    for (extension_start_index = 0; extension_start_index < filename_len; extension_start_index++) {
        if (filename[extension_start_index] == '.') {
            extension_start_index++;
            break;
        }
    }

    char *extension = slice_string(filename, extension_start_index, filename_len);
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


void print_IHDR_chunk_data(const char *path) {
    FILE *image_file = NULL;
    int c;
    int index = 0;
    int length, width, height, bit_depth, color_type, compression_method, filter_method, interlace_method;

    errno_t err = fopen_s(&image_file, path, "rb");

    // skip png signature bytes + 4 length bytes + 4 chunk type (IHDR) bytes 
    for (int i = 0; i < SIGNATURE_END_INDEX+8; i++) {
        fgetc(image_file);
    }

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

    printf("IHDR chunk data:\n");
    printf("width:\t\t\t%d\n", width);
    printf("height:\t\t\t%d\n", height);
    printf("bit depth:\t\t%d\n", bit_depth);
    printf("color type:\t\t%d\n", color_type);
    printf("compression method:\t%d\n", compression_method);
    printf("filter method:\t\t%d\n", filter_method);
    printf("interlace method:\t%d\n\n", interlace_method);

    fclose(image_file);
}

