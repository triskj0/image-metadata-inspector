#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "test.h"
#include "../src/file_operations.h"

#define RESULTS_FILE_NAME "results.txt"
#define EXIF_ITXT_PATH "../src/examples/png/exif-itxt.png"
#define ITXT_CORRECT_RESULT "\n\n\n ------------ iTXt chunk data ------------\n"\
		"\ntiff metadata\nResolutionUnit:\t\t\t2\nXResolution:\t\t\t72"\
		"\nYResolution:\t\t\t72\nOrientation:\t\t\t1\n\nxmp metadata"\
		"\nCreatorTool:\t\t\tPicsart\n\nexif metadata\nPixelXDimension:\t\t1023"\
		"\nColorSpace:\t\t\t1\nPixelYDimension:\t\t1023\n"

#define IHDR_PATH EXIF_ITXT_PATH
#define IHDR_CORRECT_RESULT "\n\n ------------ IHDR chunk data ------------ \n"\
		"width:\t\t\t\t1023\nheight:\t\t\t\t1023\nbit depth:\t\t\t8\n"\
		"color type:\t\t\t6\ncompression method:\t\t0\nfilter method:\t\t\t0\n"\
		"interlace method:\t\t0\n"


void delete_and_recreate_results_file() {
	// delete, doesn't matter wheter it is successful or not
	remove(RESULTS_FILE_NAME);
	
	// create a new results file
	FILE *new_fp = fopen(RESULTS_FILE_NAME, "w");
	fclose(new_fp);
}


void ihdr_test(int *passed_count, int *failed_count) {
	delete_and_recreate_results_file();
	int old_stdout = dup(1);

	int results_file_fd = open(RESULTS_FILE_NAME, O_WRONLY, 0666);
	dup2(results_file_fd, STDOUT_FILENO);

	// START testing inside the file
	FILE *image_fp = fopen(IHDR_PATH, "rb");
	get_print_IHDR_chunk_data(image_fp);
	fclose(image_fp);

	// reset output
	fflush(stdout);
	dup2(old_stdout, 1);
	close(results_file_fd);
	close(old_stdout);

	// load result into a buffer
	int length = 300;
	char buffer[length];
	FILE *results_fp = fopen(RESULTS_FILE_NAME, "r");
	fread(&buffer, sizeof(char), length, results_fp);
	fclose(results_fp);

	// compare
	if (STR_TEST(buffer, IHDR_CORRECT_RESULT, "IHDR - exif-itxt.png")) {
		(*passed_count)++;
		return;
	}
	(*failed_count)++;
}


void itxt_test(int *passed_count, int *failed_count) {
	fflush(stdout);
	delete_and_recreate_results_file();
	int old_stdout = dup(1);

	int results_file_fd = open(RESULTS_FILE_NAME, O_WRONLY, 0666);
	dup2(results_file_fd, STDOUT_FILENO);

	FILE *image_fp = fopen(EXIF_ITXT_PATH, "rb");
	print_iTXt_chunk_data(image_fp);
	fclose(image_fp);

	fflush(stdout);
	dup2(old_stdout, 1);
	close(results_file_fd);
	close(old_stdout);

	int length = 1200;
	char buffer[length];
	FILE *results_fp = fopen(RESULTS_FILE_NAME, "r");
	fread(&buffer, sizeof(char), length, results_fp);
	fclose(results_fp);

	if (STR_TEST(buffer, ITXT_CORRECT_RESULT, "ITXT - exif-itxt.png")) {
		(*passed_count)++;
		return;
	}
	(*failed_count)++;
}


int main(void) {

	int passed_count = 0;
	int failed_count = 0;

	// tests
	ihdr_test(&passed_count, &failed_count);
	itxt_test(&passed_count, &failed_count);

	print_results(passed_count, failed_count);

	remove(RESULTS_FILE_NAME);
	return 0;
}

