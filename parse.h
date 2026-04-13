#ifndef PARSE_H
#define PARSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * unchanged from last project
 * Reads all integers from a file into a dynamically allocated array.
 * Numbers can be separated by newlines OR commas (as the doctor specified).
 * Sets *count to the number of integers read.
 * Caller is responsible for free()-ing the returned array.
 * Returns NULL on failure.
 */
static int *read_numbers(const char *filepath, int *count) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        perror("fopen");
        return NULL;
    }

    int capacity = 64;
    int *nums = malloc(capacity * sizeof(int));
    if (!nums) { fclose(fp); return NULL; }

    *count = 0;
    char buf[256];

    // Read the whole file as text, tokenize on comma, newline, space
    while (fgets(buf, sizeof(buf), fp)) {
        char *tok = strtok(buf, ",\n\r ");
        while (tok) {
            // Skip empty tokens
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

#endif