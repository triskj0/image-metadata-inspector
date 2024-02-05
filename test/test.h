
#define TEST(got, expected, fn_name)            \
    do {                                        \
        if (got == expected) {                  \
            printf("\n[PASSED] %s", fn_name);   \
			return 1;							\
        }	      		                        \
       	printf("\n[FAILED] %s", fn_name);   	\
		return 0;								\
												\
    } while (0);


int STR_TEST(char *got, char *expected, char *fn_name) {
	if (strcmp(got, expected) == 0) {
		printf("\n[PASSED] %s", fn_name);
		return 1;
	}
	printf("[FAILED] %s", fn_name);
	return 0;
}


#define STR_TEST2(got, expected, fn_name)       \
    do {                                        \
        if (strcmp(got, expected) == 0) {       \
            (*passed_count)++;                  \
            printf("\n[PASSED] %s", fn_name);   \
        }                                       \
        else {                                  \
	    printf("\n%d", strcmp(got, expected));  \
            *(failed_count)++;                  \
            printf("\n[FAILED] %s", fn_name);   \
        }                                       \
    } while (0);


void print_results(int, int);
void find_difference(char *, char *);



void print_results(int passed, int failed) {
	printf("\n\n[TOTAL] passed: %d", passed);
	printf("\n[TOTAL] failed: %d", failed);
}


void find_difference(char *str1, char *str2) {
	int max_iter = strlen(str1) + 10;
	char *c1 = str1;
	char *c2 = str2;

	for (int i = 0; i < max_iter; i++, c1++, c2++) {
		printf("\n%c : %c", *c1, *c2);
		if (*c1 != *c2) {
			printf("\n\nHERE\n%c\n%c", *c1, *c2);
			return;
		}
	}
	printf("\ndidn't find any differences\n");
}

