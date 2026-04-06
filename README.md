# OS Lab Project 5 — IPC via Shared Memory & POSIX Threads


## Files
- `compute.h` — Header file with function declarations.
- `parse.h` — Header file for reading and parsing numbers from file.
- `sequential_compute.c` — Sequential computation implementation.
- `parallel_compute.c` — Parallel computation implementation.
- `mmap_compute.c` — Parallel computation using `fork()` + `mmap()` shared memory.
- `threads_compute.c` — Parallel computation using POSIX threads.
- `test_correctness.c` — Test program to run and verify computations.
- `program` — Compiled executable.
- `project_slides.pptx` — Project slides.

## Compilation
Run the following command to compile all sources into a single executable:

```bash
gcc sequential_compute.c parallel_compute.c mmap_compute.c threads_compute.c test_correctness.c -o program -lpthread

```
``` bash
 ./program
```
