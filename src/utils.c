#include <unistd.h>
#include <string.h>

#include "utils.h"

void log_message(LogLevel level,
                    const char *file,
                    int line,
                    const char *msg)
{
    const char *prefix;

    switch (level)
    {
        case LOG_INFO:    prefix = "[LOG] "; break;
        case LOG_WARNING: prefix = "[WARNING] "; break;
        case LOG_ERROR:   prefix = "[ERROR] "; break;
        default:          prefix = "[UNKNOWN] "; break;
    }

    write(STDERR_FILENO, prefix, strlen(prefix));
    write(STDERR_FILENO, file, strlen(file));
    write(STDERR_FILENO, ":", 1);

    char buf[16];
    int i = 15;
    buf[i--] = '\0';

    if (line == 0)
        buf[i--] = '0';
    else
    {
        while (line > 0 && i >= 0)
        {
            buf[i--] = '0' + (line % 10);
            line /= 10;
        }
    }

    write(STDERR_FILENO, buf + i + 1, strlen(buf + i + 1));
    write(STDERR_FILENO, " ", 1);

    write(STDERR_FILENO, msg, strlen(msg));
    write(STDERR_FILENO, "\n", 1);
}