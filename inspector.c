#include <stdio.h>
#include <stdlib.h>
#include "file_operations.h"


int main(int argc, char *argv[]) {

    // filename
    char *path = "";
    char *filename = get_filename(path);
    printf("\nfilename: %s\n", filename);

    
    // filetype
    char *extension = get_filetype_extension(filename);
    printf("file extension: %s\n", extension);

    // filesize
    int file_size = get_file_size(path);
    printf("file size: %d bytes", file_size);

    free(filename);
    return 0;
}

