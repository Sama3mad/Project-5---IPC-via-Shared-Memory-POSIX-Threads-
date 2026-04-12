#include "compute.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * sequential_compute
 * ------------------
 * Reads N numbers from filepath, then applies f left-to-right pairwise:
 *   result = f(f(f(nums[0], nums[1]), nums[2]), nums[3]) ...
 *
 * This matches the doctor's example:
 *   add(add(add(add(1,2),3),4),5) = 15
 *
 * Returns the final result, or 0 if the file has fewer than 1 number.
 */
int sequential_compute(const char *filepath, ulfunc_t f) {
    int count = 0;
    unsigned long *nums = read_numbers_ul(filepath, &count);
    if (!nums || count == 0) {
        free(nums);
        return 0;
    }

    // Start with the first number, fold left through the rest
    unsigned long result = nums[0];
    for (int i = 1; i < count; i++) {
        result = f(result, nums[i]);
    }

    free(nums);
    return result;
}
