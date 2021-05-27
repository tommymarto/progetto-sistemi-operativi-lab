#include <logging.h>

#include <pthread.h>

#define BLACK "\033[0;30m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define PURPLE "\033[0;35m"
#define CYAN "\033[0;36m"
#define WHITE "\033[0;37m"
#define RESET "\033[0m"

#define COLORIZE(COLOR, TEXT) COLOR TEXT RESET

int logging_level = ERROR | FATAL;

static pthread_mutex_t printMutex = PTHREAD_MUTEX_INITIALIZER;

static inline void logger(FILE* stream, const char* type, const char* fmt, va_list args) {
    if(stream == NULL || type == NULL || fmt == NULL || args == NULL) {
        return;
    }
    
    pthread_mutex_lock(&printMutex);
    
    fprintf(stream, "[%s]: ", type);
    vfprintf(stream, fmt, args);
    fprintf(stream, "\n");

    pthread_mutex_unlock(&printMutex);
}

void boring_file_log(FILE* stream, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    logger(stream, "LOG", fmt, args);
}

void log_info_stream(FILE* stream, const char* fmt, ...) {
    if(logging_level & INFO) {
        va_list args;
        va_start(args, fmt);

        logger(stream, COLORIZE(CYAN, "INFO"), fmt, args);
    }
}

void log_report_stream(FILE* stream, const char* fmt, ...) {
    if(logging_level & REPORT) {
        va_list args;
        va_start(args, fmt);

        logger(stream, COLORIZE(GREEN, "REPORT"), fmt, args);
    }
}

void log_operation_stream(FILE* stream, const char* fmt, ...) {
    if(logging_level & OPERATION) {
        va_list args;
        va_start(args, fmt);

        logger(stream, COLORIZE(BLUE, "OPERATION"), fmt, args);
    }
}

void log_warn_stream(FILE* stream, const char* fmt, ...) {
    if(logging_level & WARN) {
        va_list args;
        va_start(args, fmt);

        logger(stream, COLORIZE(YELLOW, "WARN"), fmt, args);
    }
}

void log_error_stream(FILE* stream, const char* fmt, ...) {
    if(logging_level & ERROR) {
        va_list args;
        va_start(args, fmt);

        logger(stream, COLORIZE(RED, "ERROR"), fmt, args);
    }
}

void log_fatal_stream(FILE* stream, const char* fmt, ...) {
    if(logging_level & FATAL) {
        va_list args;
        va_start(args, fmt);

        logger(stream, COLORIZE(PURPLE, "FATAL"), fmt, args);
    }
}