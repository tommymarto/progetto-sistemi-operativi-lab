#pragma once

#include <stdarg.h>
#include <stdio.h>

enum logging_type {INFO = (1<<0), OPERATION = (1<<1), REPORT = (1<<2), WARN = (1<<3), ERROR = (1<<4), FATAL = (1<<5)};

static inline void logger(FILE* stream, const char* type, const char* user_fmt, va_list args);

void boring_file_log(FILE* stream, const char* fmt, ...);

void log_info_stream(FILE* stream, const char* fmt, ...);
void log_operation_stream(FILE* stream, const char* fmt, ...);
void log_report_stream(FILE* stream, const char* fmt, ...);
void log_warn_stream(FILE* stream, const char* fmt, ...);
void log_error_stream(FILE* stream, const char* fmt, ...);
void log_fatal_stream(FILE* stream, const char* fmt, ...);

#define log_info(...) log_info_stream(stdout, __VA_ARGS__)
#define log_operation(...) log_operation_stream(stdout, __VA_ARGS__)
#define log_report(...) log_report_stream(stdout, __VA_ARGS__)
#define log_warn(...) log_warn_stream(stdout, __VA_ARGS__)
#define log_error(...) log_error_stream(stderr, __VA_ARGS__)
#define log_fatal(...) log_fatal_stream(stderr, __VA_ARGS__)