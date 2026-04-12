#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "compute.h"

#define REPEATS 23   // odd

//int fun(int a, int b) { return a * b; }

unsigned long ul_fun(unsigned long a, unsigned long b) { return a * b; }

//int sequential_compute(const char *filename, int (*f)(int, int));
//int parallel_compute(const char *filename, int n_proc, int (*f)(int, int));

// Time
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Sort helper
int cmp_double(const void *a, const void *b) {
    double x = *(double *)a;
    double y = *(double *)b;
    return (x > y) - (x < y);
}

// Median
double median(double arr[]) {
    qsort(arr, REPEATS, sizeof(double), cmp_double);
    return arr[REPEATS / 2];
}

void generate_file(const char *filename, int N) {
    FILE *fp = fopen(filename, "w");
    for (int i = 1; i <= N; i++) {
        fprintf(fp, "%d\n", i);
    }
    fclose(fp);
}

// Experiment 2
void experiment_fixed_N() {
    const char *filename = "data.txt";
    int N = 5000000;

    generate_file(filename, N);
    printf("n_proc,Sequential,Pipes,MMap,Threads\n");

    for (int n_proc = 1; n_proc <= 24; n_proc++) {
        double seq_times[REPEATS];
        double par_times[REPEATS];
        double mmap_times[REPEATS];
        double thr_times[REPEATS];

        for (int r = 0; r < REPEATS; r++) {
            double start, end;
            

            // Pipes
            start = get_time();
            parallel_compute(filename, n_proc, ul_fun);
            end = get_time();
            par_times[r] = end - start;

            // Sequential
            start = get_time();
            sequential_compute(filename, ul_fun);
            end = get_time();
            seq_times[r] = end - start;

            // MMap
            start = get_time();
            mmap_compute(filename, n_proc, ul_fun);
            end = get_time();
            mmap_times[r] = end - start;

            // Threads
            start = get_time();
            threads_compute(filename, n_proc, ul_fun);
            end = get_time();
            thr_times[r] = end - start;

        }

        double seq_ftime = median(seq_times);
        double par_ftime = median(par_times);
        double mmap_time = median(mmap_times);
        double thr_time = median(thr_times);

        printf("%d,%f,%f,%f,%f\n", n_proc, seq_ftime, par_ftime,mmap_time,thr_time);
    }
}

int main() {
    experiment_fixed_N();

    return 0;
}
