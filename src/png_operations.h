#ifndef PNG_OPERATIONS_H
#define PNG_OPERATIONS_H


char *get_filename(const char *path);
char *get_filetype_extension(const char *filename);
int get_file_size(const char *path);
char *get_last_change_date(const char *path);


// chunk-related functions
int png_get_print_IHDR_chunk_data(FILE *image_file);
void png_print_PLTE_chunk_data(FILE *image_file);
void png_print_tEXt_chunk_data(FILE *image_file);
void png_print_iTXt_chunk_data(FILE *image_file);
void png_print_bKGD_chunk_data(FILE *image_file, int color_type);
void png_print_cHRM_chunk_data(FILE *image_file);
void png_print_gAMA_chunk_data(FILE *image_file);
void png_print_pHYs_chunk_data(FILE *image_file);
void png_print_sBIT_chunk_data(FILE *image_file, int color_type);
void png_print_tRNS_chunk_data(FILE *image_file, int color_type);
void png_print_tIME_chunk_data(FILE *image_file);
void png_print_sRGB_chunk_data(FILE *image_file);
void png_search_for_common_private_chunks(FILE *image_file);
void png_print_eXIf_chunk_data(FILE *image_file);


#endif // PNG_OPERATIONS_H

