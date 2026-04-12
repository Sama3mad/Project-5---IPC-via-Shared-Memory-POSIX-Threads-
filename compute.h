#ifndef COMPUTE_H
#define COMPUTE_H

typedef int           (*func_t)(int a, int b);           // for sequential & parallel
typedef unsigned long (*ulfunc_t)(int a, int b);         // for mmap & threads

int           sequential_compute(const char *filepath, func_t f);
int           parallel_compute(const char *filepath, int n_proc, func_t f);
unsigned long mmap_compute(const char *filepath, int n_proc, ulfunc_t f);
unsigned long threads_compute(const char *filepath, int n_threads, ulfunc_t f);

#endif