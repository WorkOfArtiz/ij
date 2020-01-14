#include "logger.hpp"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* Color definitions */
#define COL_YELLOW "\033[33;1m"
#define COL_BLUE "\033[34;1m"
#define COL_RED "\033[31;1m"
#define COL_GREEN "\033[32;1m"
#define COL_RST "\033[0m"

/* Log levels */
const LogLevelImpl log_info("INFO", COL_BLUE);
const LogLevelImpl log_succ("SUCC", COL_GREEN);
const LogLevelImpl log_warn("WARN", COL_YELLOW);
const LogLevelImpl log_err("ERR", COL_RED);

/* define the global logger instance */
Logger log;

Logger::Logger(FILE *out, bool force_col) {
    _out = out == nullptr ? stderr : out;
    _col_enabled = (_out == stdout || _out == stderr || force_col);
    _log_level = LogLevel::warn;
}

Logger::~Logger() {
    if (_out != stdout && _out != stderr)
        fclose(_out);
}

void Logger::set_log_level(LogLevel level) { _log_level = level; }

void Logger::info(const char *fmt, ...) {
    if (_log_level < LogLevel::info)
        return;

    va_list args;

    va_start(args, fmt);
    log(log_info, fmt, args);
    va_end(args);
}

void Logger::success(const char *fmt, ...) {
    if (_log_level < LogLevel::success)
        return;

    va_list args;

    va_start(args, fmt);
    log(log_succ, fmt, args);
    va_end(args);
}

void Logger::warn(const char *fmt, ...) {
    if (_log_level < LogLevel::warn)
        return;

    va_list args;

    va_start(args, fmt);
    log(log_warn, fmt, args);
    va_end(args);
}

void Logger::panic(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    log(log_err, fmt, args);
    va_end(args);
    exit(1);
}

// TODO add minimal log level system in here

// explain to the compiler that this function has a
// format string and that we're not really into this
// whole format string checking. See https://stackoverflow.com/a/36120843
// for more information on this attribute.
// Also note that we index 3 here because of 2 reasons:
// 1. The format argument list starts at 1
// 2. the logger class' this argument is implicitly added.
__attribute__((__format__(__printf__, 3, 0))) void
Logger::log(const LogLevelImpl &lvl, const char *fmt, va_list args) {
    if (_col_enabled)
        fprintf(_out, "[%s%s%s] ", lvl.color, lvl.name, COL_RST);
    else
        fprintf(_out, "[%s] ", lvl.name);

    vfprintf(_out, fmt, args);
    fprintf(_out, "\n");
}
