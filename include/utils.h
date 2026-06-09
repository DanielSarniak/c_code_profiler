#ifndef UTILS_H
#define UTILS_H

#define LOG_VERBOSE

typedef enum
{
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

void log_message(LogLevel level,
                    const char *file,
                    int line,
                    const char *msg);

#define LOG_EX(level, msg) log_message(level, __FILE__, __LINE__, msg)

#define WARN(msg)  LOG_EX(LOG_WARNING, msg)
#define ERROR(msg) LOG_EX(LOG_ERROR, msg)

#ifdef LOG_VERBOSE
    #define LOG(msg) log_message(LOG_INFO, __FILE__, __LINE__, msg)
#else
    #define LOG(msg) ((void)0)
#endif

#endif /* UTILS_H */