#include "compute.h"
#include <stdio.h>
#include <stdlib.h>

/* ── func_t functions (int) — for sequential & parallel ─────────── */
int add(int a, int b)      { return a + b; }
int multiply(int a, int b) { return a * b; }
int max_f(int a, int b)    { return a > b ? a : b; }

/* ── ulfunc_t functions (unsigned long) — for mmap & threads ────── */
unsigned long ul_add(int a, int b)      { return (unsigned long)(a + b); }
unsigned long ul_multiply(int a, int b) { return (unsigned long)(a * b); }
unsigned long ul_max(int a, int b)      { return (unsigned long)(a > b ? a : b); }

/* ── Test helpers ────────────────────────────────────────────────── */
static void write_file(const char *path, int *nums, int n) {
    FILE *fp = fopen(path, "w");
    for (int i = 0; i < n; i++) {
        if (i > 0) fprintf(fp, "\n");
        fprintf(fp, "%d", nums[i]);
    }
    fclose(fp);
}

static void check_int(const char *label, int got, int expected) {
    if (got == expected)
        printf("  PASS  %-55s  got=%d\n", label, got);
    else
        printf("  FAIL  %-55s  got=%d  expected=%d\n", label, got, expected);
}

static void check_ul(const char *label, unsigned long got, unsigned long expected) {
    if (got == expected)
        printf("  PASS  %-55s  got=%lu\n", label, got);
    else
        printf("  FAIL  %-55s  got=%lu  expected=%lu\n", label, got, expected);
}

/* ── Test cases ──────────────────────────────────────────────────── */
void test_sequential() {
    printf("\n=== sequential_compute ===\n");
    int a[] = {1, 2, 3, 4, 5};
    write_file("/tmp/t1.txt", a, 5);

    check_int("add [1,2,3,4,5]",      sequential_compute("/tmp/t1.txt", add),      15);
    check_int("multiply [1,2,3,4,5]", sequential_compute("/tmp/t1.txt", multiply), 120);
    check_int("max [1,2,3,4,5]",      sequential_compute("/tmp/t1.txt", max_f),    5);

    int b[] = {42};
    write_file("/tmp/t2.txt", b, 1);
    check_int("single element 42", sequential_compute("/tmp/t2.txt", add), 42);

    int c[] = {10, 7};
    write_file("/tmp/t3.txt", c, 2);
    check_int("add [10,7]", sequential_compute("/tmp/t3.txt", add), 17);
}

void test_parallel() {
    printf("\n=== parallel_compute (pipes) ===\n");
    int a[] = {1, 2, 3, 4, 5};
    write_file("/tmp/t1.txt", a, 5);

    check_int("add [1..5] n_proc=5",      parallel_compute("/tmp/t1.txt", 5, add),      15);
    check_int("add [1..5] n_proc=2",      parallel_compute("/tmp/t1.txt", 2, add),      15);
    check_int("add [1..5] n_proc=3",      parallel_compute("/tmp/t1.txt", 3, add),      15);
    check_int("add [1..5] n_proc=1",      parallel_compute("/tmp/t1.txt", 1, add),      15);
    check_int("multiply [1..5] n_proc=5", parallel_compute("/tmp/t1.txt", 5, multiply), 120);
    check_int("multiply [1..5] n_proc=2", parallel_compute("/tmp/t1.txt", 2, multiply), 120);

    int b[] = {10, 20, 30};
    write_file("/tmp/t4.txt", b, 3);
    check_int("multiply [10,20,30] n_proc=2", parallel_compute("/tmp/t4.txt", 2, multiply), 6000);
    check_int("multiply [10,20,30] n_proc=3", parallel_compute("/tmp/t4.txt", 3, multiply), 6000);

    int c[] = {1, 2, 3, 4, 5, 6, 7};
    write_file("/tmp/t5.txt", c, 7);
    check_int("add [1..7] n_proc=3 (N not div by nproc)", parallel_compute("/tmp/t5.txt", 3, add), 28);
}

void test_mmap() {
    printf("\n=== mmap_compute ===\n");
    int a[] = {1, 2, 3, 4, 5};
    write_file("/tmp/t1.txt", a, 5);

    check_ul("add [1..5] n_proc=1",      mmap_compute("/tmp/t1.txt", 1, ul_add), 15);
    check_ul("add [1..5] n_proc=2",      mmap_compute("/tmp/t1.txt", 2, ul_add), 15);
    check_ul("add [1..5] n_proc=3",      mmap_compute("/tmp/t1.txt", 3, ul_add), 15);
    check_ul("add [1..5] n_proc=5",      mmap_compute("/tmp/t1.txt", 5, ul_add), 15);
    check_ul("multiply [1..5] n_proc=1", mmap_compute("/tmp/t1.txt", 1, ul_multiply), 120);
    check_ul("multiply [1..5] n_proc=2", mmap_compute("/tmp/t1.txt", 2, ul_multiply), 120);
    check_ul("multiply [1..5] n_proc=5", mmap_compute("/tmp/t1.txt", 5, ul_multiply), 120);
    check_ul("max [1..5] n_proc=3",      mmap_compute("/tmp/t1.txt", 3, ul_max), 5);

    int b[] = {10, 20, 30};
    write_file("/tmp/t4.txt", b, 3);
    check_ul("multiply [10,20,30] n_proc=2", mmap_compute("/tmp/t4.txt", 2, ul_multiply), 6000);
    check_ul("multiply [10,20,30] n_proc=3", mmap_compute("/tmp/t4.txt", 3, ul_multiply), 6000);

    int c[] = {1, 2, 3, 4, 5, 6, 7};
    write_file("/tmp/t5.txt", c, 7);
    check_ul("add [1..7] n_proc=3 (N not div by nproc)", mmap_compute("/tmp/t5.txt", 3, ul_add), 28);

    int d[] = {99};
    write_file("/tmp/t7.txt", d, 1);
    check_ul("single element 99",             mmap_compute("/tmp/t7.txt", 1, ul_add), 99);
    check_ul("add [1..5] n_proc=10 (clamped)", mmap_compute("/tmp/t1.txt", 10, ul_add), 15);
}

void test_threads() {
    printf("\n=== threads_compute ===\n");
    int a[] = {1, 2, 3, 4, 5};
    write_file("/tmp/t1.txt", a, 5);

    check_ul("add [1..5] n_threads=1",      threads_compute("/tmp/t1.txt", 1, ul_add), 15);
    check_ul("add [1..5] n_threads=2",      threads_compute("/tmp/t1.txt", 2, ul_add), 15);
    check_ul("add [1..5] n_threads=3",      threads_compute("/tmp/t1.txt", 3, ul_add), 15);
    check_ul("add [1..5] n_threads=5",      threads_compute("/tmp/t1.txt", 5, ul_add), 15);
    check_ul("multiply [1..5] n_threads=1", threads_compute("/tmp/t1.txt", 1, ul_multiply), 120);
    check_ul("multiply [1..5] n_threads=2", threads_compute("/tmp/t1.txt", 2, ul_multiply), 120);
    check_ul("multiply [1..5] n_threads=5", threads_compute("/tmp/t1.txt", 5, ul_multiply), 120);
    check_ul("max [1..5] n_threads=3",      threads_compute("/tmp/t1.txt", 3, ul_max), 5);

    int b[] = {10, 20, 30};
    write_file("/tmp/t4.txt", b, 3);
    check_ul("multiply [10,20,30] n_threads=2", threads_compute("/tmp/t4.txt", 2, ul_multiply), 6000);
    check_ul("multiply [10,20,30] n_threads=3", threads_compute("/tmp/t4.txt", 3, ul_multiply), 6000);

    int c[] = {1, 2, 3, 4, 5, 6, 7};
    write_file("/tmp/t5.txt", c, 7);
    check_ul("add [1..7] n_threads=3 (N not div by nthreads)", threads_compute("/tmp/t5.txt", 3, ul_add), 28);

    int d[] = {99};
    write_file("/tmp/t7.txt", d, 1);
    check_ul("single element 99",                threads_compute("/tmp/t7.txt", 1, ul_add), 99);
    check_ul("add [1..5] n_threads=10 (clamped)", threads_compute("/tmp/t1.txt", 10, ul_add), 15);
}

void test_all_agree() {
    printf("\n=== mmap vs threads must agree ===\n");
    int nums[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    write_file("/tmp/t8.txt", nums, 10);

    for (int n = 1; n <= 5; n++) {
        unsigned long mm = mmap_compute("/tmp/t8.txt", n, ul_add);
        unsigned long tt = threads_compute("/tmp/t8.txt", n, ul_add);
        char label[80];
        snprintf(label, sizeof(label), "add [1..10] mmap n=%d", n);
        check_ul(label, mm, 55);
        snprintf(label, sizeof(label), "add [1..10] threads n=%d", n);
        check_ul(label, tt, 55);
    }

    for (int n = 1; n <= 4; n++) {
        unsigned long mm = mmap_compute("/tmp/t8.txt", n, ul_multiply);
        unsigned long tt = threads_compute("/tmp/t8.txt", n, ul_multiply);
        char label[80];
        snprintf(label, sizeof(label), "multiply [1..10] mmap n=%d", n);
        check_ul(label, mm, 3628800);
        snprintf(label, sizeof(label), "multiply [1..10] threads n=%d", n);
        check_ul(label, tt, 3628800);
    }
}

void test_both_agree() {
    printf("\n=== sequential vs parallel — must produce identical results ===\n");
    int nums[] = {-3, -7, -2, -9, -1, -5, -8, -4, -6, -10};
    write_file("/tmp/t6.txt", nums, 10);

    for (int np = 1; np <= 5; np++) {
        int s = sequential_compute("/tmp/t6.txt", add);
        int p = parallel_compute("/tmp/t6.txt", np, add);
        char label[64];
        snprintf(label, sizeof(label), "add [-3..-10] n_proc=%d", np);
        check_int(label, p, s);
    }
}

int main() {
    test_sequential();
    test_parallel();
    test_mmap();
    test_threads();
    test_all_agree();
    //test_both_agree();

    printf("\nDone.\n");
    return 0;
}