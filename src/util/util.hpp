#ifndef UTIL_H
#define UTIL_H
#include <sstream>
#include <string>
#include <array>
#include <vector>
#include <algorithm>
#include "logger.hpp"

/* in function */
template <class T, class S>
constexpr inline bool in(const T &value, std::initializer_list<S> options) {
    for (const T &i : options)
        if (value == i)
            return true;

    return false;
}

static inline bool endswith(const std::string &string, const std::string &ending)
{
    size_t slen = string.length();
    size_t elen = ending.length();

    return elen <= slen && std::string::npos != string.rfind(ending, slen - elen);
}

/* optimised contains, for containers with and without find */
template <class C, class T>
inline auto contains_impl(const C &c, const T &x, int)
    -> decltype(c.find(x), true) {
    return end(c) != c.find(x);
}

template <class C, class T>
inline bool contains_impl(const C &v, const T &x, long) {
    return end(v) != std::find(begin(v), end(v), x);
}

template <class C, class T>
auto contains(const C &c, const T &x) -> decltype(end(c), true) {
    return contains_impl(c, x, 0);
}

template <class T> inline int indexOf(std::vector<T> &v, const T &value) {
    auto x = std::find(begin(v), end(v), value);
    if (end(v) == x)
        return -1;

    return x - begin(v);
}

template <class T> inline void push_front(std::vector<T> v, T t) {
    v.push_back(t);
    std::rotate(v.rbegin(), v.rbegin() + 1, v.rend());
}

template <class Container, class Text>
static inline std::string join(Text delim, Container values) {
    std::stringstream s;

    for (auto iter = values.begin(); iter != values.end();) {
        s << *iter;
        if (++iter != values.end())
            s << delim;
    }

    return s.str();
}

static inline void concat_helper(std::string &) { return; }

template <class Stringy, class... Stringies>
static inline void concat_helper(std::string &s, Stringy &t,
                                 Stringies... other) {
    s += t;
    concat_helper(s, other...);
}

template <class... Strinigies>
static inline std::string concat(Strinigies... args) {
    std::string s;
    s.reserve(sizeof...(args) * 10);
    concat_helper(s, args...);
    return s;
}

static inline void sprint_helper(std::stringstream &s, const char *fmt) {
    s << fmt;
}

template <typename ArgType, typename... ArgTypes>
static inline void sprint_helper(std::stringstream &out, const char *fmt,
                                 ArgType arg, ArgTypes... args) {
    for (const char *s = fmt; *s; s++) {
        if (*s == '%' && *(++s) != '%') {
            switch (*s) {
            case 'd':
                out << arg;
                break;
            case 's':
                out << arg;
                break;
            case 'x':
                out << std::hex << arg;
                break;
            default:
                throw std::runtime_error{std::string("unsupported format: ") +
                                         *s};
            }

            sprint_helper(out, s + 1, args...);
            break;
        } else
            out << *s;
    }
}

template <typename... ArgTypes>
static std::string sprint(const char *fmt, ArgTypes... args) {
    std::stringstream s;
    sprint_helper(s, fmt, args...);
    return s.str();
}

template <typename T> struct option {
    inline option(option<T> &&other) : store{other.store} {
        other.store = nullptr;
    }
    inline option() : store{nullptr} {}
    inline option(const T &v) : store{new T{v}} {}
    inline ~option() {
        if (store)
            delete store;
    }
    inline bool isset() { return store != nullptr; };
    inline operator const T &() const {
        if (!store)
            log.panic("Trying to cast from empty option");

        return *store;
    }

  private:
    T *store;
};

bool in(std::string needle, std::initializer_list<std::string> hay);

#endif