#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#define MAX_TESTS 256

static const char *tests[MAX_TESTS];
static int test_count = 0;

void load_tests()
{
    DIR *d = opendir("build");
    if (!d) {
        perror("build");
        exit(1);
    }

    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_type == DT_REG) {
            if (strstr(e->d_name, "test")) {
                tests[test_count++] = strdup(e->d_name);
            }
        }
    }

    closedir(d);
}

void list_tests()
{
    printf("Usage: ./c_code_profiler <test case>\n\n");
    printf("Available tests:\n");
    for (int i = 0; i < test_count; i++) {
        printf("  %s\n", tests[i]);
    }
}

void run_test(const char *name)
{
    char path[256];
    snprintf(path, sizeof(path), "%s", name);

    printf("Running: %s\n", path);

    setenv("LD_PRELOAD", "./build/libprofiler.so", 1);

    execl(path, path, NULL);

    perror("execl failed");
}

int main(int argc, char **argv)
{
    load_tests();

    if (argc < 2) {
        list_tests();
        return 0;
    }

    if (strcmp(argv[1], "list") == 0) {
        list_tests();
        return 0;
    }

    run_test(argv[1]);
}