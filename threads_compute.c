#include "compute.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/*
 * threads_compute
 * ---------------
 * Splits the N numbers across n_threads POSIX threads as evenly as possible.
 * The remainder (N % n_threads) is given to the LAST thread.
 *
 * Design:
 *   - A shared array  partials[n_threads]  holds each thread's result.
 *   - Each thread receives a thread_arg_t describing its slice.
 *   - No mutex needed: each thread writes to a distinct index in partials[].
 *
 * Flow:
 *   1. Read all numbers into a local array.
 *   2. Allocate partials[] and args[].
 *   3. pthread_create n_threads threads.
 *   4. pthread_join all threads.
 *   5. Aggregate partials[] with f left-to-right.
 *   6. Free and return result.
 */

typedef struct {
    unsigned long *nums;
    int            start;   // start index of this thread's slice
    int            end;     // end index of this thread's slice
    ulfunc_t       f;       // function to apply
    unsigned long *result;  // pointer to partials[i], thread writes here
} thread_arg_t;

// Thread worker function
static void *thread_worker(void *arg) {
    thread_arg_t *a = (thread_arg_t *)arg;
    unsigned long partial = a->nums[a->start]; // initialize with first element
    for (int k = a->start + 1; k <= a->end; k++)
        partial = a->f(partial, a->nums[k]);  // apply f over the slice
    *(a->result) = partial;                    // store partial result
    return NULL;                               // thread exits
}

unsigned long threads_compute(const char *filepath, int n_threads, ulfunc_t f) {
    int count = 0;
    unsigned long *nums = read_numbers_ul(filepath, &count); // read numbers from file
    if (!nums || count == 0) {
        free(nums); // nothing to compute
        return 0;
    }

    if (n_threads > count) n_threads = count; // don't create more threads than numbers

    // Allocate shared structures
    unsigned long *partials = malloc(n_threads * sizeof(unsigned long)); // store each thread's result
    thread_arg_t  *args     = malloc(n_threads * sizeof(thread_arg_t));   // args for each thread
    pthread_t     *tids     = malloc(n_threads * sizeof(pthread_t));     // thread IDs
    if (!partials || !args || !tids) {
        free(nums); free(partials); free(args); free(tids);
        return 0; // allocation failed
    }

    int chunk     = count / n_threads; // base number of elements per thread
    int remainder = count % n_threads; // extra elements for the last thread

    for (int i = 0; i < n_threads; i++) {
        args[i].nums   = nums;                // shared numbers array
        args[i].start  = i * chunk;           // start index for this thread
        args[i].end    = args[i].start + chunk - 1; // end index for this thread
        args[i].f      = f;                   // function to apply
        args[i].result = &partials[i];        // store result in partials[i]
        if (i == n_threads - 1) args[i].end += remainder; // last thread takes remainder

        if (pthread_create(&tids[i], NULL, thread_worker, &args[i]) != 0) {
            perror("pthread_create");         // thread creation failed
            free(nums); free(partials); free(args); free(tids);
            return 0;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < n_threads; i++)
        pthread_join(tids[i], NULL);

    // Aggregate partial results
    unsigned long result = partials[0];
    for (int i = 1; i < n_threads; i++)
        result = f(result, partials[i]); // combine all thread results

    // Cleanup
    free(nums); free(partials); free(args); free(tids);
    return result; // return final aggregated result
}