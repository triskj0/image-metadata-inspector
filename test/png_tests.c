#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "test.h"
#include "../src/file_operations.h"

#define RESULTS_FILE_NAME "results.txt"
#define IHDR_PATH "../src/examples/png/exif-itxt.png"
#define IHDR_CORRECT_RESULT "\n\n ------------ IHDR chunk data ------------ \n"\
		"width:\t\t\t\t1023\nheight:\t\t\t\t1023\nbit depth:\t\t\t8\n"\
		"color type:\t\t\t6\ncompression method:\t\t0\nfilter method:\t\t\t0\n"\
		"interlace method:\t\t0\n"

#define EXIF_ITXT_PATH IHDR_PATH
#define ITXT_MESSAGE "ITXT - exif-itxt.png"
#define ITXT_CORRECT_RESULT "\n\n\n ------------ iTXt chunk data ------------\n"\
		"\ntiff metadata\nResolutionUnit:\t\t\t2\nXResolution:\t\t\t72"\
		"\nYResolution:\t\t\t72\nOrientation:\t\t\t1\n\nxmp metadata"\
		"\nCreatorTool:\t\t\tPicsart\n\nexif metadata\nPixelXDimension:\t\t1023"\
		"\nColorSpace:\t\t\t1\nPixelYDimension:\t\t1023\n"

#define TEXT_PATH "../src/examples/png/sbit-phys-text-prvw-mkts.png"
#define TEXT_MESSAGE "TEXT - sbit-phys-text-prvw-mkts.png"
#define TEXT_CORRECT_RESULT "\n\n\n ------------ tEXt chunk data ------------\n"\
		"Software\t\t\tAdobe Fireworks CS6\n"

#define PLTE_PATH "../src/examples/png/gama-srgb-phys-plte.png"
#define PLTE_MESSAGE "PLTE - gama-srgb-phys-plte.png"
#define PLTE_CORRECT_RESULT "\n\n\n ------------ PLTE chunk data ------------\n"\
		"palette length:\t\t\t768\nnumber of colors:\t\t256\n"

#define BKGD_PATH "../src/examples/png/gama-chrm-phys-time-bkgd-text.png"
#define BKGD_MESSAGE "BKGD - gama-chrm-phys-time-bkgd-text.png"
#define BKGD_CORRECT_RESULT "\n\n\n ------------ bKGD chunk data ------------\n"\
		"background color value:\t\t[255, 255, 255]\n"

int color_type;


void delete_and_recreate_results_file() {
	// delete, doesn't matter wheter it is successful or not
	remove(RESULTS_FILE_NAME);
	
	// create a new results file
	FILE *new_fp = fopen(RESULTS_FILE_NAME, "w");
	fclose(new_fp);
}


void reset_out(int old_stdout_fd) {
	fflush(stdout);
	dup2(old_stdout_fd, 1);
	close(old_stdout_fd);
}


// returns file descriptor to results file
int redirect_stdout() {
	int results_fd = open(RESULTS_FILE_NAME, O_WRONLY, 0666);
	dup2(results_fd, STDOUT_FILENO);
	return results_fd;
}


int read_file(char *buffer, FILE *file_ptr) {
	int c;
	int i = 0;

	while ((c = fgetc(file_ptr)) != EOF) {
		buffer[i] = c;
		++i;
	}

	return i;
}


void compare_result(char *result_buffer, int n_bytes_read, char *correct_result, char *description, int *passed_count, int *failed_count) {
	int index = 0;
	int correct_result_length = 0;

	char failed_message[50] = "[FAILED] ";
	char passed_message[50] = "[PASSED] ";
	strcat(failed_message, description);
	strcat(passed_message, description);

	// get length of correct_result
	for (int j = 0; correct_result[j] != 0; j++)
		correct_result_length++;

	for (; index < correct_result_length; index++) {
		if (result_buffer[index] != correct_result[index]) {
			printf("\n%s", failed_message);
			(*failed_count)++;
			return;
		}
	}
	if (n_bytes_read == index) {
		printf("\n%s", passed_message);
		//printf("\nbytes read: %d", index);
		(*passed_count)++;
		return;
	}

	printf("\n%s", failed_message);
	(*failed_count)++;
}


void ihdr_test(int *passed_count, int *failed_count) {
	delete_and_recreate_results_file();
	int old_stdout = dup(1);

	int results_file_fd = redirect_stdout();

	// START testing inside the file
	FILE *image_fp = fopen(IHDR_PATH, "rb");
	color_type = get_print_IHDR_chunk_data(image_fp);
	fclose(image_fp);

	// reset output
	reset_out(old_stdout);
	close(results_file_fd);

	// load result into a buffer
	int length = 200;
	char buffer[length];
	FILE *results_fp = fopen(RESULTS_FILE_NAME, "r");
	int n_bytes_read = read_file(buffer, results_fp);
	fclose(results_fp);

	compare_result(buffer, n_bytes_read, IHDR_CORRECT_RESULT, "IHDR - exif-itxt.png", passed_count, failed_count);
}


void test_fn(void (*function_ptr)(), int buffer_length, char *image_path, char *correct_result,\
	char *test_message, int *passed_count, int *failed_count) {

	fflush(stdout);
	delete_and_recreate_results_file();
	int old_stdout = dup(1);

	int results_file_fd = redirect_stdout();

	FILE *image_fp = fopen(image_path, "rb");

	if (strcmp(test_message, BKGD_MESSAGE) == 0) {
		(*function_ptr) (image_fp, color_type);
	}
	else {
		(*function_ptr) (image_fp);
	}

	fclose(image_fp);
	reset_out(old_stdout);
	close(results_file_fd);

	char buffer[buffer_length];
	FILE *results_fp = fopen(RESULTS_FILE_NAME, "r");
	int n_bytes_read = read_file(buffer, results_fp);
	fclose(results_fp);

	compare_result(buffer, n_bytes_read, correct_result, test_message, passed_count, failed_count);
}


int main(void) {

	int passed_count = 0;
	int failed_count = 0;

	// tests
	ihdr_test(&passed_count, &failed_count);

	test_fn(print_iTXt_chunk_data, 300, EXIF_ITXT_PATH, ITXT_CORRECT_RESULT,\
			ITXT_MESSAGE, &passed_count, &failed_count);

	test_fn(print_tEXt_chunk_data, 150, TEXT_PATH, TEXT_CORRECT_RESULT,\
			TEXT_MESSAGE, &passed_count, &failed_count);

	test_fn(print_PLTE_chunk_data, 150, PLTE_PATH, PLTE_CORRECT_RESULT,\
			PLTE_MESSAGE, &passed_count, &failed_count);

	test_fn(print_bKGD_chunk_data, 150, BKGD_PATH, BKGD_CORRECT_RESULT,\
			BKGD_MESSAGE, &passed_count, &failed_count);

	print_results(passed_count, failed_count);
	remove(RESULTS_FILE_NAME);
	return 0;
}

