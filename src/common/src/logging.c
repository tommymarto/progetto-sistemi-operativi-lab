#include <logging.h>

int logging_level = ERROR;

static inline void logger(FILE* stream, const char* type, const char* fmt, va_list args) {
    fprintf(stream, "[%s]: ", type);
    vfprintf(stream, fmt, args);
    fprintf(stream, "\n");
}

void log_info(FILE* stream, const char* fmt, ...) {
    if(logging_level & INFO) {
        va_list args;
        va_start(args, fmt);

        logger(stream, "INFO", fmt, args);
    }
}

void log_error(FILE* stream, const char* fmt, ...) {
    if(logging_level & ERROR) {
        va_list args;
        va_start(args, fmt);

        logger(stream, "ERROR", fmt, args);
    }
}

void log_operation(FILE* stream, const char* fmt, ...) {
    if(logging_level & OPERATION) {
        va_list args;
        va_start(args, fmt);

        logger(stream, "OPERATION", fmt, args);
    }
}