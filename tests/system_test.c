#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

int main() {
    printf("\n--- Test file operatiton (and printf) ---\n");
    FILE *file = fopen("test_system_allocs.tmp", "w+");
    if (file != NULL) {
        fprintf(file, "Force getdelim.\n");
        fflush(file);
        fclose(file);
    }

    printf("--- Test file reading (getline) ---\n");
    file = fopen("test_system_allocs.tmp", "r");
    if (file != NULL) {
        char *line = NULL;
        size_t len = 0;
        
        if (getline(&line, &len, file) != -1) {
            printf("Odczytano: %s", line);
        }
    
        free(line); 
        fclose(file);
    }
    
    remove("test_system_allocs.tmp");
}