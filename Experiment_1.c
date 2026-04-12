#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "compute.h"

#define NUM_READINGS 100
#define REPEATS 7            // for smoothing

// Function pointer
int fun(int a, int b) { return a * b; }

unsigned long ul_fun(int a, int b) { return a * b; }



//int sequential_compute(const char *filename, int (*f)(int, int));
//int parallel_compute(const char *filename, int n_proc, int (*f)(int, int));

// Time helper
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int cmp_double(const void *a, const void *b) {
    double x = *(double *)a;
    double y = *(double *)b;
    if (x < y) return -1;
    if (x > y) return 1;
    return 0;
}

// Generate input file with N numbers
void generate_file(const char *filename, int step) {
    FILE *fp = fopen(filename, "a");
    if (!fp) {
        perror("File error");
        exit(1);
    }

    for (int i = 1; i <= step; i++) {
        fprintf(fp, "%d\n", i);
    }

    fclose(fp);
}

// Experiment: Fix n_proc, vary N
void experiment_fixed_nproc(int n_proc) {
    const char *filename = "data.txt";
    FILE *fp = fopen(filename, "w");
    fclose(fp);


    printf("N,Sequential,Pipes,MMap,Threads \n");

    for (int i = 1; i <= NUM_READINGS; i++) {
        int step = 500000;       // edit as needed
        int N = i * step;

        generate_file(filename, step);

        double seq_times[REPEATS];
        double par_times[REPEATS];
        double mmap_times[REPEATS];
        double thr_times[REPEATS];

        for (int r = 0; r < REPEATS; r++) {
            double start, end;

            // Sequential timing
            start = get_time();
            sequential_compute(filename, fun);
            end = get_time();
            seq_times[r] = (end - start);

            // Pipes timing
            start = get_time();
            parallel_compute(filename, n_proc, fun);
            end = get_time();
            par_times[r] = (end - start);

            // MMap timing
            start = get_time();
            mmap_compute(filename, n_proc, ul_fun);
            end = get_time();
            mmap_times[r] = (end - start);

            // Threads timing
            start = get_time();
            threads_compute(filename, n_proc, ul_fun);
            end = get_time();
            thr_times[r] = (end - start);
        }

        // Median
        qsort(seq_times, REPEATS, sizeof(double), cmp_double);
        qsort(par_times, REPEATS, sizeof(double), cmp_double);
        qsort(mmap_times, REPEATS, sizeof(double), cmp_double);
        qsort(thr_times, REPEATS, sizeof(double), cmp_double);

        double seq_time, par_time, mmap_time, thr_time;

        seq_time = seq_times[REPEATS/2];
        par_time = par_times[REPEATS/2];
        mmap_time = mmap_times[REPEATS/2];
        thr_time = thr_times[REPEATS/2];
    

        printf("%d,%f,%f, %f, %f\n", N, seq_time, par_time, mmap_time, thr_time);
    }
}

int main() {
    experiment_fixed_nproc(8);
    return 0;
}
