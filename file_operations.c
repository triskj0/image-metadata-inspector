#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "file_operations.h"


// index2 is non-inclusive
char *slice_string(char *str, int index1, int index2) {
    int new_str_len = index2 - index1;

    char *new_str = malloc(new_str_len*sizeof(char));
    int new_str_index = 0;
    int str_index = index1;

    for (; new_str_index <= new_str_len; ) {
        new_str[new_str_index++] = str[str_index++];
    }

    return new_str;
}


char *get_filename(char *path) {
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


char *get_filetype_extension(char *filename) {
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

