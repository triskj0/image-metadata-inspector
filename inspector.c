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
    printf("\nfilename: %s\n", filename);

    extension = get_filetype_extension(filename);
    printf("file extension: %s\n", extension);

    file_size = get_file_size(path);
    printf("file size: %d bytes\n", file_size);


    // image height


    // image width

    
    last_change_date = get_last_change_date(path);
    printf("last change date: %s\n", last_change_date);



    free(filename);
    free(last_change_date);
    return 0;
}

