#ifndef PARSE_H
#define PARSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * Reads all integers (including negatives) from a file into an int array.
 * Used by sequential_compute and parallel_compute.
 */
static int *read_numbers(const char *filepath, int *count) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) { perror("fopen"); return NULL; }

    int capacity = 64;
    int *nums = malloc(capacity * sizeof(int));
    if (!nums) { fclose(fp); return NULL; }

    *count = 0;
    char buf[256];

    while (fgets(buf, sizeof(buf), fp)) {
        char *tok = strtok(buf, ",\n\r ");
        while (tok) {
            int valid = 0;
            for (int i = 0; tok[i]; i++)
                if (isdigit(tok[i]) || tok[i] == '-') { valid = 1; break; }

            if (valid) {
                if (*count == capacity) {
                    capacity *= 2;
                    int *tmp = realloc(nums, capacity * sizeof(int));
                    if (!tmp) { free(nums); fclose(fp); return NULL; }
                    nums = tmp;
                }
                nums[(*count)++] = atoi(tok);
            }
            tok = strtok(NULL, ",\n\r ");
        }
    }

    fclose(fp);
    return nums;
}

/*
 * Reads non-negative numbers from a file into an unsigned long array.
 * Used by mmap_compute and threads_compute.
 *
 * unsigned long cannot represent negative numbers.
 * Negative values in the file are skipped with a warning printed to stderr.
 * This is correct behavior — mmap_compute and threads_compute are defined
 * to return unsigned long, so the input domain must be non-negative.
 */
static unsigned long *read_numbers_ul(const char *filepath, int *count) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) { perror("fopen"); return NULL; }

    int capacity = 64;
    unsigned long *nums = malloc(capacity * sizeof(unsigned long));
    if (!nums) { fclose(fp); return NULL; }

    *count = 0;
    char buf[256];

    while (fgets(buf, sizeof(buf), fp)) {
        char *tok = strtok(buf, ",\n\r ");
        while (tok) {
            // Negative numbers cannot be stored as unsigned long — skip and warn
            if (tok[0] == '-') {
                fprintf(stderr,
                    "Warning: negative number '%s' skipped "
                    "(mmap/threads_compute requires unsigned values)\n", tok);
                tok = strtok(NULL, ",\n\r ");
                continue;
            }

            int valid = 0;
            for (int i = 0; tok[i]; i++)
                if (isdigit(tok[i])) { valid = 1; break; }

            if (valid) {
                if (*count == capacity) {
                    capacity *= 2;
                    unsigned long *tmp = realloc(nums, capacity * sizeof(unsigned long));
                    if (!tmp) { free(nums); fclose(fp); return NULL; }
                    nums = tmp;
                }
                nums[(*count)++] = strtoul(tok, NULL, 10);
            }
            tok = strtok(NULL, ",\n\r ");
        }
    }

    fclose(fp);
    return nums;
}

#endif