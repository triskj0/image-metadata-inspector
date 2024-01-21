#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <stdio.h>

#define SIGNATURE_END_INDEX 8
#define IHDR_LENGTH 13


char *get_filename(const char *path);

char *get_filetype_extension(const char *filename);

int get_file_size(const char *path);

char *get_last_change_date(const char *path);


// chunk-related functions
int get_print_IHDR_chunk_data(FILE *image_file);

void print_PLTE_chunk_data(FILE *image_file);

void print_tEXt_chunk_data(FILE *image_file);

void print_iTXt_chunk_data(FILE *image_file);

void print_bKGD_chunk_data(FILE *image_file, int color_type);

void print_cHRM_chunk_data(FILE *image_file);


#endif // FILE_OPERATIONS_H

