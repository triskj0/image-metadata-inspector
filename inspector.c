#include <stdio.h>
#include <stdlib.h>
#include "file_operations.h"


int main(int argc, char **argv) {
    
    if (argc < 2) {
        printf("\n[ERROR] Please provide a path to a PNG image.\n");
        exit(EXIT_FAILURE);
    }

    const char *path = argv[1];
    const char *extension;
    char *filename;
    int file_size;
    char *last_change_date;
    FILE *image_file = NULL;

    filename = get_filename(path);
    extension = get_filetype_extension(filename);

    printf("\n\nfilename:\t\t%s\n", filename);
    printf("file extension:\t\t%s\n", extension);
    free(filename);

    file_size = get_file_size(path);
    printf("file size:\t\t%d bytes\n", file_size);

    last_change_date = get_last_change_date(path);
    printf("last change date:\t%s\n", last_change_date);
    free(last_change_date);


    // read chunk data
    errno_t err = fopen_s(&image_file, path, "rb");
    print_IHDR_chunk_data(path, image_file);

    print_PLTE_chunk_data(image_file);

    print_tEXt_chunk_data(image_file);
    

    fclose(image_file);
    return EXIT_SUCCESS;
}

