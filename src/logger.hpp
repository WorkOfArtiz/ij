/**************************************************
 * Logger utility
 *************************************************/
#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <stdarg.h>

/* internal use */
struct LogLevel
{
    LogLevel(const char *name, const char *color) : name{name}, color{color} {}
    const char *name;
    const char *color;
};

/* provide in to string for any vector */
template<typename T>
std::ostream& operator<<(std::ostream &out, const std::vector<T> &v)
{
    out << '[';

    for (typename std::vector<T>::const_iterator i = v.begin(); i != v.end(); ++i)
    {
        out << *i;

        if (i + 1 != v.end())
            out << ", ";
    }

    out << ']';
    return out;
}

/* helper to convert objects to std::strings */
template<typename T>
std::string str(const T &obj)
{
    std::ostringstream stream;
    stream << obj;
    return stream.str();
}

#define cstr(obj) str(obj).c_str()

class Logger
{
    public:
        Logger(FILE *out=nullptr, bool force_col=false);
        ~Logger();

        void info(const char *fmt, ...);
        void warn(const char *fmt, ...);
        void success(const char *fmt, ...);
        void panic(const char *fmt, ...);

    private:
        void log(const LogLevel &lvl, const char *fmt, va_list args);

        FILE *_out;
        bool _col_enabled;
};

/* Global logger instance */
extern Logger log;
