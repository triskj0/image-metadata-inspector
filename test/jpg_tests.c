#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "test.h"
#include "../src/jpg_operations.h"

#define RESULTS_FILE_NAME "results.txt"
#define IFD_MESSAGE_1M "IFD data - 1m.jpg"
#define IFD_PATH_1M "../src/examples/jpg/1m.jpg"
#define IFD_CORRECT_RESULT_1M "byte alignment:\t\t\tMM (Big Endian/Motorola)\n"\
    "image width: \t\t\t4000\nimage length: \t\t\t3000\nbits per sample: \t\t8, 8, 8\nmake: \t\t\t\tHUAWEI\0\n"\
    "model: \t\t\t\tMAR-LX1B\0\norientation: \t\t\tunknown\nx resolution: \t\t\t72/1\ny resolution: \t\t\t72/1\n"\
    "resolution unit: \t\tinches\nsoftware: \t\t\tMAR-L21B 10.0.0.563(C431E7R4P1)\0\ndateTime: \t\t\t2022:08:02 18:02:26\0\n"\
    "YCbCr positioning: \t\tcentered\nexif tag: \t\t\t286\nGPS tag: \t\t\t8962\ndevice setting description: \t1768976384\n"\
    "compression: \t\t\tJPEG (old-style)\nJPEG interchange format: \t9318\nJPEG interchange format length: 34098\n"
#define SEGMENT_MARKERS_CORRECT_RESULT_1M "jfif version:\t\t\t1.01\n\nother present segments include:\n"\
    " - quantisation table (0xFFD8)\n - start of baseline DCT frame (0xFFC0)\n - huffman table (0xFFC4)"\
    "\n - start of scan (0xFFDA)\n\n"
#define SEGMENT_MARKERS_MESSAGE_1M "Segment Markers Data - 1m.jpg"

#define IFD_MESSAGE_3M "IFD data - 3m.jpg"
#define IFD_PATH_3M "../src/examples/jpg/3m.jpg"
#define SEGMENT_MARKERS_CORRECT_RESULT_3M "jfif version:\t\t\t1.02\n\nother present segments include:\n"\
    " - quantisation table (0xFFD8)\n - start of baseline DCT frame (0xFFC0)\n - huffman table (0xFFC4)"\
    "\n - start of scan (0xFFDA)\n\n"
#define SEGMENT_MARKERS_MESSAGE_3M "Segment Markers Data - 3m.jpg"


void delete_and_recreate_results_file()
{
    remove(RESULTS_FILE_NAME);

    FILE *new_fp = fopen(RESULTS_FILE_NAME, "w");
    fclose(new_fp);
}


void reset_out(int old_stdout_fd)
{
    fflush(stdout);
    dup2(old_stdout_fd, 1);
    close(old_stdout_fd);
}


int redirect_stdout()
{
    int results_fd = open(RESULTS_FILE_NAME, O_WRONLY, 0666);
    dup2(results_fd, STDOUT_FILENO);
    return results_fd;
}


int read_file(char *buffer, FILE *file_ptr)
{
	int c;
	int i = 0;

	while ((c = fgetc(file_ptr)) != EOF) {
		buffer[i] = c;
		++i;
    }
    return i;
}


void compare_result(char *result_buffer, int n_bytes_read, char *correct_result, char *description,\
    int *passed_count, int *failed_count)
{
    int index = 0;

    char failed_message[50] = "[FAILED] ";
    char passed_message[50] = "[PASSED] ";
    strcat(failed_message, description);
    strcat(passed_message, description);

    for (; index < n_bytes_read; index++) {
        if (result_buffer[index] != correct_result[index]) {
            printf("\n%s", failed_message);
            (*failed_count)++;
            return;
        }
    }
    if (n_bytes_read == index) {
        printf("\n%s", passed_message);
        (*passed_count)++;
        return;
    }
    
    printf("\n%s", failed_message);
    (*failed_count)++;
}


void test_fn(void (*function_ptr)(), int buffer_length, char *image_path, char *correct_result,\
    char *test_message, int *passed_count, int *failed_count)
{
    fflush(stdout);
    delete_and_recreate_results_file();
    int old_stdout = dup(1);

    int results_file_fd = redirect_stdout();

    FILE *image_fp = fopen(image_path, "rb");
    (*function_ptr) (image_fp);
    fclose(image_fp);

    reset_out(old_stdout);
    close(results_file_fd);

    char buffer[buffer_length];
    FILE *results_fp = fopen(RESULTS_FILE_NAME, "r");
    int n_bytes_read = read_file(buffer, results_fp);
    fclose(results_fp);

    compare_result(buffer, n_bytes_read, correct_result, test_message, passed_count, failed_count);
}


int main(void)
{
    int passed_count = 0;
    int failed_count = 0;

    test_fn(jpg_print_segment_markers_data, 300, IFD_PATH_1M, SEGMENT_MARKERS_CORRECT_RESULT_1M,\
            SEGMENT_MARKERS_MESSAGE_1M, &passed_count, &failed_count);

    test_fn(jpg_print_exif_data, 800, IFD_PATH_1M, IFD_CORRECT_RESULT_1M,\
            IFD_MESSAGE_1M, &passed_count, &failed_count);

    test_fn(jpg_print_segment_markers_data, 300, IFD_PATH_3M, SEGMENT_MARKERS_CORRECT_RESULT_3M,\
            SEGMENT_MARKERS_MESSAGE_3M, &passed_count, &failed_count);


    print_results(passed_count, failed_count);
    //remove(RESULTS_FILE_NAME);

    return 0;
}
