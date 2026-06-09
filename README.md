gcc -shared -fPIC -o libprofiler.so profiler.c -ldl -pthread
LD_PRELOAD=./libprofiler.so ./a