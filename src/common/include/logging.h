#pragma once

#include <stdarg.h>
#include <stdio.h>

enum logging_type {INFO = (1<<0), OPERATION = (1<<1), MESSAGE = (1<<2), ERROR = (1<<3), FATAL = (1<<4)};

static inline void logger(FILE* stream, const char* type, const char* user_fmt, va_list args);

void log_info_stream(FILE* stream, const char* fmt, ...);
void log_operation_stream(FILE* stream, const char* fmt, ...);
void log_message_stream(FILE* stream, const char* fmt, ...);
void log_error_stream(FILE* stream, const char* fmt, ...);
void log_fatal_stream(FILE* stream, const char* fmt, ...);

#define log_info(...) log_info_stream(stdout, __VA_ARGS__)
#define log_operation(...) log_operation_stream(stdout, __VA_ARGS__)
#define log_message(...) log_message_stream(stdout, __VA_ARGS__)
#define log_error(...) log_error_stream(stderr, __VA_ARGS__)
#define log_fatal(...) log_fatal_stream(stderr, __VA_ARGS__)