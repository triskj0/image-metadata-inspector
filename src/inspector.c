#ifdef _WIN32
#   define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "png_operations.h"
#include "jpg_operations.h"

#define USAGE "\n[USAGE] %s <path to image>\n\n"
#define FILE_NOT_FOUND_ERROR "\n[ERROR] Invalid file-name, file was not found.\n\n"
#define COULD_NOT_OPEN_FILE_ERROR "[ERROR] file could not be opened.\n\n"


void print_jpg_metadata(FILE *image_file)
{
    jpg_print_exif_data(image_file);
}


void print_png_metadata(FILE *image_file)
{

    int color_type = png_get_print_IHDR_chunk_data(image_file);
    png_print_cHRM_chunk_data(image_file);
    png_print_gAMA_chunk_data(image_file);
    png_print_sBIT_chunk_data(image_file, color_type);
    png_print_PLTE_chunk_data(image_file);
    png_print_tRNS_chunk_data(image_file, color_type);
    png_print_bKGD_chunk_data(image_file, color_type);
    png_print_tEXt_chunk_data(image_file);
    png_print_iTXt_chunk_data(image_file);
    png_print_pHYs_chunk_data(image_file);
    png_print_sRGB_chunk_data(image_file);
    png_print_tIME_chunk_data(image_file);
    png_print_eXIf_chunk_data(image_file);
    png_search_for_common_private_chunks(image_file);

}


int main(int argc, char **argv)
{
    
    if (argc < 2) {
        fprintf(stderr, USAGE, get_filename(argv[0]));
        exit(EXIT_FAILURE);
    }

    const char *path = argv[1];
    char *extension;
    char *filename;
    int file_size;
    char *last_change_date;
    FILE *image_file;

    filename = get_filename(path);
    extension = get_filetype_extension(filename);

    printf("\n\nfilename:\t\t\t%s\n", filename);
    printf("file extension:\t\t\t%s\n", extension);
    free(filename);

    file_size = get_file_size(path);

    if (file_size == -1) {
        fprintf(stderr, FILE_NOT_FOUND_ERROR);
        exit(EXIT_FAILURE);
    }

    printf("file size:\t\t\t%d bytes\n", file_size);

    last_change_date = get_last_change_date(path);
    printf("last change date:\t\t%s\n", last_change_date);
    free(last_change_date);


    // read chunk data
    image_file = fopen(path, "rb");

    if (image_file == NULL) {
        fprintf(stderr, COULD_NOT_OPEN_FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    if (strcmp(extension, "jpg") == 0 || strcmp(extension, "JPG") == 0)
        print_jpg_metadata(image_file);
    else print_png_metadata(image_file);

    free(extension);
    fclose(image_file);
    printf("\n\n");

    return EXIT_SUCCESS;
}

