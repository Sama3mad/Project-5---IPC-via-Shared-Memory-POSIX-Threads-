#ifndef COMPUTE_H
#define COMPUTE_H

// For sequential_compute and parallel_compute (pipes) — from previous project
//typedef int (*func_t)(int a, int b);

// For mmap_compute and threads_compute — returns unsigned long
typedef unsigned long (*ulfunc_t)(unsigned long a, unsigned long b);

// Previous project (unchanged)
int sequential_compute(const char *filepath, ulfunc_t f);
int parallel_compute(const char *filepath, int n_proc, ulfunc_t f);

// New project functions
unsigned long mmap_compute(const char *filepath, int n_proc, ulfunc_t f);
unsigned long threads_compute(const char *filepath, int n_threads, ulfunc_t f);

#endif