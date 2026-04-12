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
    int           *nums;
    int            start;
    int            end;
    ulfunc_t       f;
    unsigned long *result;
} thread_arg_t;

static void *thread_worker(void *arg) {
    thread_arg_t *a = (thread_arg_t *)arg;
    unsigned long partial = (unsigned long)a->nums[a->start];
    for (int k = a->start + 1; k <= a->end; k++)
        partial = a->f((int)partial, a->nums[k]);  // f takes int, int
    *(a->result) = partial;
    return NULL;
}

unsigned long threads_compute(const char *filepath, int n_threads, ulfunc_t f) {
    int count = 0;
    int *nums = read_numbers(filepath, &count);  // int* now
    if (!nums || count == 0) {
        free(nums);
        return 0;
    }

    if (n_threads > count) n_threads = count;

    unsigned long *partials = malloc(n_threads * sizeof(unsigned long));
    thread_arg_t  *args     = malloc(n_threads * sizeof(thread_arg_t));
    pthread_t     *tids     = malloc(n_threads * sizeof(pthread_t));
    if (!partials || !args || !tids) {
        free(nums); free(partials); free(args); free(tids);
        return 0;
    }

    int chunk     = count / n_threads;
    int remainder = count % n_threads;

    for (int i = 0; i < n_threads; i++) {
        args[i].nums   = nums;
        args[i].start  = i * chunk;
        args[i].end    = args[i].start + chunk - 1;
        args[i].f      = f;
        args[i].result = &partials[i];
        if (i == n_threads - 1) args[i].end += remainder;

        if (pthread_create(&tids[i], NULL, thread_worker, &args[i]) != 0) {
            perror("pthread_create");
            free(nums); free(partials); free(args); free(tids);
            return 0;
        }
    }

    for (int i = 0; i < n_threads; i++)
        pthread_join(tids[i], NULL);

    unsigned long result = partials[0];
    for (int i = 1; i < n_threads; i++)
        result = f((int)result, (int)partials[i]);  // f takes int, int

    free(nums); free(partials); free(args); free(tids);
    return result;
}