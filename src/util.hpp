#ifndef UTIL_H
#define UTIL_H
#include <sstream>
#include <array>
#include <vector>
#include <algorithm>

/* in function */
template <class T>
constexpr inline bool in(const T &value, std::initializer_list<T> options) {
    for (const T &i : options)
        if (value == i)
            return true;

    return false;
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

template <class Container>
static inline std::string join(std::string delim, Container values) {
    std::stringstream s;

    for (auto iter = values.begin(); iter != values.end();) {
        s << *iter;
        if (++iter != values.end())
            s << delim;
    }

    return s.str();
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
#endif