#ifdef _WIN32
#   define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int csv_count_lines(FILE *fp)
{
    int c;
    int total = 0;

    while ((c = fgetc(fp)) != EOF) {
        if (c == '\n') total++;
    }

    fseek(fp, 0, SEEK_SET);
    return total+1;
}


static void _goto_next_line(FILE *csv_fp)
{
    char c;

    while ((c = fgetc(csv_fp)) != '\n' && c != EOF) {
        ;
    }
}


static int _get_nth_power(int number, int power)
{
    if (power == 0)
        return 1;

    else if (power == 1)
        return number;

    int original_number = number;
    for (int i = power; i > 1; i--) {
        number *= original_number;
    }
    return number;
}


static int _get_int_from_hex_char(char str_number)
{
    if (str_number >= '0' && str_number <= '9') {
        return str_number - '0';
    }

    else if (str_number >= 'A' && str_number <= 'F') {
        return str_number - 'A' + 10;
    }

    else if (str_number >= 'a' && str_number <= 'f') {
        return str_number - 'a' + 10;
    }

    printf("%c  ", str_number);
    return -1;
}


// fp must be right before the first character
static int _read_key(FILE *csv_fp)
{
    int c;
    int total = 0;

    for (int i = 4; i > 0; i--) {
        char str_number = fgetc(csv_fp);
        c = _get_int_from_hex_char(str_number);
        
        if (c == -1) {
            printf("%c\n", c);
            fprintf(stderr, "[ERROR] Could not decode hex tag number from"
                    " file storing exif tags.\n%s : %d\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }

        total += c * _get_nth_power(16, i-1);
    }

    return total;
}


// fp must be right before the value
static char *_read_value(FILE *csv_fp)
{
    char c;
    int i;
    char *value = malloc(50 * sizeof(char));

    fseek(csv_fp, 1, SEEK_CUR); // skip ':'
    for (i = 0; (c = fgetc(csv_fp)) != '\n' && c != EOF; i++) {
        value[i] = c;
    }
    value[i+1] = 0;

    return value;
}


char *csv_get_string_by_value(FILE *csv_fp, int key)
{
    fseek(csv_fp, 0, SEEK_SET);

    int i;
    int csv_number_of_lines = csv_count_lines(csv_fp);

    for (i = 0; i < csv_number_of_lines; i++) {
        int key_from_file = _read_key(csv_fp);

        if (key_from_file == key) {
            break;
        }

        _goto_next_line(csv_fp);
    }

    if (i == csv_number_of_lines) {
        return "";
    }

    char *value = _read_value(csv_fp);

    for (int k = 0; ; k++) {
        if (value[k] == 0) {
            break;
        }
        
        if (value[k] == 13) {
            value[k] = 0;
            break;
        }
    }

    return value;
}

