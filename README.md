# c_code_profiler - Lightweight Thread-Safe Memory Profiler

C_code_profiler is a lightweight, thread-safe memory profiler for Linux systems, written in C. The tool uses the LD_PRELOAD mechanism to intercept memory allocation calls on the fly, allowing for precise tracking of Memory Leaks in user applications.

This project was built to gain a deep understanding of memory management, multithreading in C, and the architecture of process address spaces in UNIX-like systems.

## Key Features

* **Zero-dependency:** Requires no code instrumentation, no recompilation of the profiled program, and no external libraries (aside from the standard library).
* **Thread-Safe:** Fully immune to race conditions in multithreaded environments thanks to granular locking (a hash table with independent mutexes).
* **Reentrancy safe:** Utilizes thread-local storage (_Thread_local) to safely distinguish the profiled program's allocations from the profiler's own internal allocations.
* **Whitelisting (O(1) filtering):** The profiler parses the kernel's virtual memory map (/proc/self/maps) at startup to create a whitelist of addresses. Using pointer math, the profiler ignores internal glibc allocations (e.g., printf, fopen) and other external shared libraries in O(1) time.
* **Precise Reporting:** Distinguishes between actual memory leaks caused by developer errors and intentional memory retention typical for system tools.

## Under the Hood

The profiler overrides standard memory management functions (malloc, calloc, realloc, free) by utilizing function hooking via the dynamic linker (dlsym(RTLD_NEXT)).

The core architectural breakthrough is the user code isolation mechanism:
1. During initialization, the profiler reads /proc/self/exe to resolve the absolute path of the running executable.
2. It then scans /proc/self/maps to record the exact virtual memory boundaries where the pure application code is loaded.
3. Whenever an allocation function is called, the profiler uses __builtin_return_address(1) to trace the origin of the request. If the return address falls within the whitelisted boundaries, the allocation is logged. Otherwise, it is transparently passed to the system, eliminating false positives from system libraries.



## Build and Usage

### Prerequisites
* Linux OS (or WSL)
* GCC or Clang compiler

### Compiling the Profiler
To compile the profiler as a shared object library (.so), run the following command:

```bash
cmake -S . -B build
cmake --build build
./build/c_code_profiler <program>
```
or

```bash
cmake -S . -B build
cmake --build build
LD_PRELOAD=./build/libprofiler.so <program>
```

## Sample report
```bash
 ========================================
       C_CODE_PROFILER REPORT             
 ========================================
 Application name:  simple_calloc_test 
 Application path:  /home/linux/repo/c_code_profiler/build/simple_calloc_test 

 Application code ranges in memory (function addresses):
 0x600CBF48F000  - 0x600CBF490000 
 0x600CBF490000  - 0x600CBF491000 
 0x600CBF491000  - 0x600CBF492000 
 0x600CBF492000  - 0x600CBF493000 
 0x600CBF493000  - 0x600CBF494000 
 ----------------------------------------
 [LEAK] Address: 0x600CCE2B0900  | Size: 400  bytes
 ----------------------------------------
 Total leaks found: 1 
 Total memory leaked: 400  bytes
 Peak memory usage: 400  bytes
 ========================================
```

## Test Suite and Results

To demonstrate the capabilities of c_code_profiler, the repository includes a tests/ directory containing various programs designed to test different aspects of memory management.

The table below summarizes the expected output for the provided test cases, illustrating how the profiler distinguishes between actual leaks and intentional system-level allocations.

| Test Case | Total Leaks Found | Total Memory Leaked (Bytes) | Peak memory usage (Bytes)|
| :--- | :--- | :--- | :--- |
| 5000_threads_test | 0 | 0 | 256 |
|early_return_calloc_test| 1 | 60 | 60 |
|early_return_malloc_test|1|1024|1024|
|function_leak_calloc_test|1|3|3|
|function_leak_malloc_test|1|500|500|
|global_pointer_calloc_test|1|4000|4000|
|global_pointer_malloc_test|1|1000|1000|
|half_threads_valid_test|50|51200|51200|
|linked_list_calloc_test|1|16|32|
|linked_list_malloc_test|1|16|32|
|loosing_the_pointer_calloc_test|1|40|120|
|loosing_the_pointer_malloc_test|1|1000|3000|
|multithread_common_array_test|0|0|6400000|
|overwriting_realloc_test|1|80|200|
|realloc_fail_test|1|100|100|
|simple_calloc_test|1|400|400|
|simple_malloc_test|1|4|8|
|simple_multithread_test|0|0|~5500|
|simple_realloc_test|1|28|28|
|thread_for_malloc_thread_for_free_test|0|0|12800000|
|system_test|0|0|0|

The profiler reports 0 memory leaks for system_test because it strictly filters all allocations based on the user-code whitelist established via /proc/self/maps. While the glibc library and the operating system allocate and release thousands of bytes of memory to handle these operations, they are completely ignored by the profiler. Since the code explicitly frees all memory it allocates (e.g., the buffer used by getline), the profiler correctly identifies the memory lifecycle as perfectly balanced.

### Case Studies: `ls` vs `sqlite3`

To fully understand the profiler's output, it is highly recommended to run it against two real-world applications that represent opposite ends of the memory management spectrum:

#### 1. GNU `ls` (Expected: Hundreds of Leaks / Still Reachable)
If you run `LD_PRELOAD=./libprofiler.so ls`, the profiler will report a significant amount of unreleased memory (often over 100KB across hundreds of blocks). This is **not a bug in the profiler, nor a bug in `ls`**. 
Programs from the GNU Coreutils family are heavily optimized for execution speed and have very short lifespans. Instead of wasting CPU cycles iteratively calling `free()` for thousands of file descriptors and string formatting structures, `ls` delegates the cleanup to the Linux kernel, which instantly reclaims the entire virtual address space upon process `exit()`. The profiler correctly identifies this memory as allocated by the user application but never explicitly freed.

#### 2. `sqlite3` CLI (Expected: 0 Leaks)
If you run `LD_PRELOAD=./build/libprofiler.so sqlite3 :memory: "CREATE TABLE t(x); INSERT INTO t VALUES(1); SELECT * FROM t;"` you will see **exactly 0 bytes leaked**. 
SQLite is renowned for its aviation-grade testing and strictly adheres to a "zero-leak" policy. It meticulously frees every single byte of memory it allocates before shutting down. Getting a perfect zero on `sqlite3` is a good confirmation that c_code_profiler works correctly.
