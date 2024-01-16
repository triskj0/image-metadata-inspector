#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#define SIGNATURE_END_INDEX 8
#define IHDR_LENGTH 13


char *slice_string(const char *str, int index1, int index2);

char *get_filename(const char *path);

char *get_filetype_extension(const char *filename);

int get_file_size(const char *path);

char *get_last_change_date(const char *path);

void print_IHDR_chunk_data(const char *path);

#endif

