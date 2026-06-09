gcc -shared -fPIC -o libprofiler.so profiler.c -ldl -pthread
gcc main.c
LD_PRELOAD=./libprofiler.so ./a

cmake -S . -B build
cmake --build build