#include <stdio.h>
#include <stdlib.h>
#include "file_operations.h"


int main(int argc, char *argv[]) {
    const char *path, *extension;
    char *filename;
    int file_size;
    char *last_change_date;

    path = "";
    filename = get_filename(path);
    printf("\nfilename:\t\t%s\n", filename);

    extension = get_filetype_extension(filename);
    printf("file extension:\t\t%s\n", extension);

    file_size = get_file_size(path);
    printf("file size:\t\t%d bytes\n", file_size);


    last_change_date = get_last_change_date(path);
    printf("last change date:\t%s\n", last_change_date);

    print_IHDR_chunk_data(path);

    free(filename);
    free(last_change_date);
    return 0;
}

