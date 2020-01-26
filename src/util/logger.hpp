/**************************************************
 * Logger utility
 *************************************************/
#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <stdarg.h>

enum class LogLevel {
    info = 3,
    success = 2,
    warn = 1,
    panic = 0,
};

/* internal use */
struct LogLevelImpl {
    LogLevelImpl(const char *name, const char *color)
        : name{name}, color{color} {}
    const char *name;
    const char *color;
};

/* provide in to string for any vector */
template <typename T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &v) {
    out << '[';

    for (typename std::vector<T>::const_iterator i = v.begin(); i != v.end();
         ++i) {
        out << *i;

        if (i + 1 != v.end())
            out << ", ";
    }

    out << ']';
    return out;
}

/* helper to convert objects to std::strings */
template <typename T> std::string str(const T &obj) {
    std::ostringstream stream;
    stream << obj;
    return stream.str();
}

#define cstr(obj) str(obj).c_str()

class Logger {
  public:
    Logger(FILE *out = nullptr, bool force_col = false);
    ~Logger();

    void info(const char *fmt, ...);
    void warn(const char *fmt, ...);
    void success(const char *fmt, ...);
    void panic(const char *fmt, ...);

    void set_log_level(LogLevel level);
    inline const LogLevel &get_log_level() const { return _log_level; }

  private:
    void log(const LogLevelImpl &lvl, const char *fmt, va_list args);

    FILE *_out;
    bool _col_enabled;
    LogLevel _log_level;
};

/* Global logger instance */
extern Logger log;
