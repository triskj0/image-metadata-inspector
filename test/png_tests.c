#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "test.h"
#include "../src/file_operations.h"

#define RESULTS_FILE_NAME "results.txt"
#define EXIF_ITXT_PATH "../src/examples/png/exif-itxt.png"
#define ITXT_CORRECT_RESULT "\n\n\n ------------ iTXt chunk data ------------\n\ntiff metadata\nResolutionUnit:\t\t\t2\nXResolution:\t\t\t72\nYResolution:\t\t\t72\nOrientation:\t\t\t1\n\nxmp metadata\nCreatorTool:\t\t\tPicsart\n\nexif metadata\nPixelXDimension:\t\t1023\nColorSpace:\t\t\t1\nPixelYDimension:\t\t1023\n"


void delete_and_recreate_results_file() {
	// delete, doesn't matter wheter it is successful or not
	remove("results.txt");
	
	// create a new results file
	FILE *new_fp = fopen(RESULTS_FILE_NAME, "w");
	fclose(new_fp);
}


void itxt_test(int *passed_count, int *failed_count) {
	delete_and_recreate_results_file();

	// open file
	int file_descriptor = open(RESULTS_FILE_NAME , O_WRONLY | O_APPEND);

	if (file_descriptor < 0)
		printf("Could not open file");


	// redirect output
	int current_out = dup(1);
	if (dup2(file_descriptor, 1) < 0) {
		fprintf(stderr, "couldn't redirect output\n");
		return;
	}

	// print that shit into results file
	FILE *image_fp = fopen(EXIF_ITXT_PATH, "rb");
	print_iTXt_chunk_data(image_fp);
	fclose(image_fp);

	// reset output, load results.txt into a buffer, compare
	fflush(stdout);
	if (dup2(current_out, 1) < 0) {
		fprintf(stderr, "couldn't reset output\n");
		return;
	}

	FILE *fp = fopen(RESULTS_FILE_NAME, "rb");
	int length = 1000 * sizeof(char);
	char *buffer = malloc(length);
	fread(buffer, 1, length, fp);
	fclose(fp);

	STR_TEST(buffer, ITXT_CORRECT_RESULT, "ITXT - exif-itxt.png");
	print_results(*passed_count, *failed_count);
}


int main(void) {

	int passed_count = 0;
	int failed_count = 0;

	// tests
	itxt_test(&passed_count, &failed_count);
	
	remove(RESULTS_FILE_NAME);
	return 0;
}

