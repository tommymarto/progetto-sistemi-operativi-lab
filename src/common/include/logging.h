#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

static inline void log_operation(FILE* stream, const char* type, const char* user_fmt, va_list args) {
    // setup format string
    int type_len = strlen(type);
    int user_fmt_len = strlen(user_fmt);
    int fmt_len = type_len + user_fmt_len + 6; // []: '\0' = 6 extra chars
    char* fmt = (char*)malloc(fmt_len*sizeof(char));
    sprintf(fmt, "[%s]: %s\n", type, user_fmt);
    
    vfprintf(stream, fmt, args);
    free(fmt);
}

static inline void log_info(FILE* stream, const char* fmt, va_list args) {
    log_operation(stream, "INFO", fmt, args);
}

static inline void log_info_stdout(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    log_info(stdout, fmt, args);
}