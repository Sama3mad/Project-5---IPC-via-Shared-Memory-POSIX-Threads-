#include "compute.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>

/*
 * mmap_compute
 * ------------
 * Splits the N numbers across n_proc child processes as evenly as possible.
 * The remainder (N % n_proc) is given to the LAST child.
 *
 * Shared memory design:
 *   - One mmap'd array of n_proc ints: partials[i] is written by child i.
 *   - No pipes needed; children write directly into the shared region.
 *
 * Flow:
 *   1. Read all numbers into a local array (parent).
 *   2. mmap a shared array of n_proc ints (MAP_SHARED | MAP_ANONYMOUS).
 *   3. Fork n_proc children.
 *      - Each child computes its slice with f, stores result in partials[i].
 *      - Child exits.
 *   4. Parent waits for all children.
 *   5. Parent aggregates partials[0..n_proc-1] with f left-to-right.
 *   6. munmap and return result.
 */

unsigned long mmap_compute(const char *filepath, int n_proc, ulfunc_t f) {
    int count = 0;
    unsigned long *nums = read_numbers_ul(filepath, &count); // Read numbers from file
    if (!nums || count == 0) {
        free(nums); // Nothing to compute
        return 0;
    }

    if (n_proc > count) n_proc = count; // Avoid creating more processes than numbers

    // Shared memory: one unsigned long per child to hold its partial result
    unsigned long *partials = mmap(NULL, n_proc * sizeof(unsigned long),
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED | MAP_ANONYMOUS,
                                   -1, 0);
    if (partials == MAP_FAILED) {
        perror("mmap"); // mmap failed
        free(nums);
        return 0;
    }

    int chunk     = count / n_proc;      // Base number of elements per child
    int remainder = count % n_proc;      // Extra elements for the last child

    for (int i = 0; i < n_proc; i++) {
        int start = i * chunk;           // Start index for this child
        int end   = start + chunk - 1;   // End index for this child
        if (i == n_proc - 1) end += remainder; // Last child takes remainder

        pid_t pid = fork();               // Create child process
        if (pid < 0) {
            perror("fork");               // Fork failed
            munmap(partials, n_proc * sizeof(unsigned long));
            free(nums);
            return 0;
        }

        if (pid == 0) {
            /* ===== CHILD ===== */
            unsigned long partial = nums[start]; // Initialize partial result
            for (int k = start + 1; k <= end; k++)
                partial = f(partial, nums[k]);  // Apply function f over slice

            partials[i] = partial;              // Write result to shared memory

            free(nums);                         // Cleanup in child
            munmap(partials, n_proc * sizeof(unsigned long));
            exit(0);                            // Exit child
            /* ===== END CHILD ===== */
        }
    }

    for (int i = 0; i < n_proc; i++)
        wait(NULL); // Wait for all child processes to finish

    // Aggregate partial results in parent
    unsigned long result = partials[0];
    for (int i = 1; i < n_proc; i++)
        result = f(result, partials[i]); // Combine partial results

    munmap(partials, n_proc * sizeof(unsigned long)); // Cleanup shared memory
    free(nums);                                      // Free original array
    return result;                                   // Return final result
}