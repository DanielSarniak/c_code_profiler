## How to use
cmake -S . -B build
cmake --build build
./build/c_code_profiler simple_test

## Caveats & Internal glibc Allocations

When profiling a program that uses standard I/O functions like `printf()`, you might notice a persistent "leak" of exactly 1024 or 4096 bytes (depending on your system's page and buffer size), even if your code properly frees all user-allocated memory.

### Why does this happen?
The first time `printf()` is called, the GNU C Library (`glibc`) automatically initializes and allocates an internal static buffer for `stdout` to optimize I/O throughput (block/line buffering). For performance reasons, `glibc` does not explicitly call `free()` on this internal buffer when the process exits, relying on the OS kernel to clean up the process's entire address space.

Since our profiler hooks `malloc` and runs its leak check in a destructor, this internal buffer is captured and reported as a memory leak. This is an expected behavior of the C standard library.

TODO:
Add internal functions filtering

TMP SOLUTION:
setvbuf(stdout, NULL, _IONBF, 0); in profled program 
