#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"

void log_message(LogLevel level,
                    const char *file,
                    int line,
                    const char *msg)
{
    const char *prefix;
    int fileno;

    switch (level)
    {
        case LOG_INFO:
            prefix = "[LOG] ";
            fileno = STDOUT_FILENO;
            break;
        case LOG_WARNING:
            prefix = "[WARNING] ";
            fileno = STDERR_FILENO;
            break;
        case LOG_ERROR:
            prefix = "[ERROR] ";
            fileno = STDERR_FILENO;
            break;
        case LOG_PRINT:
            prefix = "";
            fileno = STDOUT_FILENO;
            break;
        default:
            prefix = "[UNKNOWN] ";
            fileno = STDOUT_FILENO;
            break;
    }

    write(fileno, prefix, strlen(prefix));
    if( level != LOG_PRINT)
    {
        write(fileno, file, strlen(file));
        write(fileno, ":", 1);
        print_num(line, 10);
    }

    write(fileno, " ", 1);

    write(fileno, msg, strlen(msg));
    if( level != LOG_PRINT)
    {
        write(fileno, "\n", 1);
    }
}

void print_num(uintptr_t num, int base) {
    char buf[32];
    int i = 0;
    
    if (num == 0) {
        PRINT("0");
        return;
    }

    while (num > 0) {
        int rem = num % base;
        buf[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'A');
        num /= base;
    }

    for (int j = i - 1; j >= 0; j--) {
        write(2, &buf[j], 1);
    }
}