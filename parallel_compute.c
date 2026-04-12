#include "compute.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/*
 * parallel_compute
 * ----------------
 * Splits the N numbers across n_proc child processes as evenly as possible.
 * The remainder (N % n_proc) is given to the LAST child.
 *
 * Pipe design (2 pipes per child):
 *   ctrl_pipe[i] : parent writes (start, end) indices → child reads them
 *   res_pipe[i]  : child writes its partial result   → parent reads it
 *
 * Flow:
 *   1. Read all numbers into an array (in the parent).
 *   2. Create 2*n_proc pipes.
 *   3. Fork n_proc children.
 *      - Each child closes the ends it doesn't need.
 *      - Child reads its (start, end) from ctrl_pipe.
 *      - Child applies f on nums[start..end] sequentially.
 *      - Child writes the partial result to res_pipe.
 *      - Child exits.
 *   4. Parent closes the ends it doesn't need, then waits for all children.
 *   5. Parent reads all partial results from res_pipes.
 *   6. Parent applies f on the partial results to get the final answer.
 */
int parallel_compute(const char *filepath, int n_proc, func_t f) {
    // --- Step 1: read all numbers ---
    int count = 0;
    int *nums = read_numbers(filepath, &count);
    if (!nums || count == 0) {
        free(nums);
        return 0;
    }

    // If n_proc is larger than count, clamp it
    if (n_proc > count) n_proc = count;

    // --- Step 2: allocate pipes ---
    // ctrl_pipe[i][0] = read end (child reads),  ctrl_pipe[i][1] = write end (parent writes)
    // res_pipe[i][0]  = read end (parent reads),  res_pipe[i][1] = write end (child writes)
    int (*ctrl_pipe)[2] = malloc(n_proc * sizeof(int[2]));
    int (*res_pipe)[2]  = malloc(n_proc * sizeof(int[2]));
    if (!ctrl_pipe || !res_pipe) {
        free(nums); free(ctrl_pipe); free(res_pipe);
        return 0;
    }

    for (int i = 0; i < n_proc; i++) {
        if (pipe(ctrl_pipe[i]) < 0 || pipe(res_pipe[i]) < 0) {
            perror("pipe");
            free(nums); free(ctrl_pipe); free(res_pipe);
            return 0;
        }
    }

    // --- Step 3: compute slice sizes ---
    int chunk    = count / n_proc;       // base slice size
    int remainder = count % n_proc;      // extra numbers go to the last child

    // --- Step 4: fork children ---
    for (int i = 0; i < n_proc; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            free(nums); free(ctrl_pipe); free(res_pipe);
            return 0;
        }

        if (pid == 0) {
            /* ===== CHILD ===== */

            // Close all pipe ends that don't belong to this child
            for (int j = 0; j < n_proc; j++) {
                // ctrl_pipe: child only reads from its own pipe
                close(ctrl_pipe[j][1]);          // parent's write end
                if (j != i) close(ctrl_pipe[j][0]);  // other children's read ends

                // res_pipe: child only writes to its own pipe
                close(res_pipe[j][0]);           // parent's read end
                if (j != i) close(res_pipe[j][1]);   // other children's write ends
            }

            // Read (start, end) from the control pipe
            int start, end;
            read(ctrl_pipe[i][0], &start, sizeof(int));
            read(ctrl_pipe[i][0], &end,   sizeof(int));
            close(ctrl_pipe[i][0]);

            // Apply f on nums[start..end] left-to-right
            int partial = nums[start];
            for (int k = start + 1; k <= end; k++) {
                partial = f(partial, nums[k]);
            }

            // Send partial result back to parent
            write(res_pipe[i][1], &partial, sizeof(int));
            close(res_pipe[i][1]);

            free(nums);
            free(ctrl_pipe);
            free(res_pipe);
            exit(0);
            /* ===== END CHILD ===== */
        }
    }

    /* ===== PARENT ===== */

    // Send each child its (start, end) and close the ends parent doesn't need
    for (int i = 0; i < n_proc; i++) {
        int start = i * chunk;
        int end   = start + chunk - 1;

        // Last child gets the remainder
        if (i == n_proc - 1) end += remainder;

        // Write start and end to the control pipe for child i
        write(ctrl_pipe[i][1], &start, sizeof(int));
        write(ctrl_pipe[i][1], &end,   sizeof(int));
        close(ctrl_pipe[i][1]);   // done writing to this child
        close(ctrl_pipe[i][0]);   // parent doesn't read from ctrl_pipe

        // Parent doesn't write to result pipe
        close(res_pipe[i][1]);
    }

    // Wait for all children to finish
    for (int i = 0; i < n_proc; i++) {
        wait(NULL);
    }

    // Collect partial results and aggregate with f
    int *partials = malloc(n_proc * sizeof(int));
    for (int i = 0; i < n_proc; i++) {
        read(res_pipe[i][0], &partials[i], sizeof(int));
        close(res_pipe[i][0]);
    }

    // Aggregate: apply f on all partial results left-to-right
    int result = partials[0];
    for (int i = 1; i < n_proc; i++) {
        result = f(result, partials[i]);
    }

    free(partials);
    free(nums);
    free(ctrl_pipe);
    free(res_pipe);
    return result;
}