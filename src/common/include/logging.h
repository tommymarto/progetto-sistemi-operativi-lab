#pragma once

#include <stdarg.h>
#include <stdio.h>

enum logging_type {INFO = (1<<0), OPERATION = (1<<1), ERROR = (1<<2)};

static inline void logger(FILE* stream, const char* type, const char* user_fmt, va_list args);
void log_info(FILE* stream, const char* fmt, ...);
void log_error(FILE* stream, const char* fmt, ...);
void log_operation(FILE* stream, const char* fmt, ...);