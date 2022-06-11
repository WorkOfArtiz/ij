#pragma once
#include "types.h"
#include "endian.hpp"
#include <ostream>

/*******************************************************************************
 * Abstraction over simple buffers:
 * - adds iteration
 * - auto reallocation
 * - generic extending for your own object marshalling
 * - appending deals with endianess
 * - keeps easy access with [] operator
 *
 * And allows for passive memory management:
 *
 * buffer get_buffer
 * {
 *    buffer b;
 *    return b;
 * }
 ******************************************************************************/
class buffer {
    friend class buffer_reader;

  public:
    /***************************************************************************
     * Basic interface
     **************************************************************************/
    buffer(unsigned capacity = 1024);
    buffer(const buffer &b); /* copy more easily */
    buffer(const buffer &b, size_t from, size_t to); /* create a slice */
    ~buffer();

    void clear() { _size = 0; }             /* destroys contents */
    unsigned size() const { return _size; } /* returns size */

    /* overloaded operators */
    void operator=(buffer &&b);                 /* pass around more easily */
    const u8 &operator[](unsigned index) const; /* access elements easily */

    /* append more easily */
    void raw_append(const u8 *raw, unsigned size);

    /* gives string escaped view of buffer */
    buffer escape();

    // These are implicit through the template append function
    // void append(char c);
    // void append(const char *s);
    // void append(const buffer &b);
    // void append(i8-i32) possibly with endianess
    // void append(u8-u32) possibly with endianess

    /***************************************************************************
     * Advanced interface
     **************************************************************************/

    /* Endianess support here is optional */
    template <typename T> void append(T value, Endian e = sys_endianess);

    /* Endianess support here optional too */
    template <typename T>
    void write(T value, u32 offset, Endian e = sys_endianess);

    /* making buffers simple to iterate over as byte arrays */
    struct iter : public std::iterator<std::input_iterator_tag, u8> {
        iter(const iter &original) : _buf{original._buf}, _i{original._i} {}
        iter(const buffer &b, int index) : _buf{b}, _i{index} {}

        u8 operator*() const { return _buf[_i]; }
        const iter &operator++() {
            ++_i;
            return *this;
        }
        iter operator++(int) {
            iter copy{*this};
            ++_i;
            return copy;
        }

        bool operator==(const iter &o) const { return _i == o._i; }
        bool operator!=(const iter &o) const { return _i != o._i; }

      private:
        const buffer &_buf;
        int _i;
    };

    buffer::iter begin() const { return buffer::iter(*this, 0); }
    buffer::iter end() const { return buffer::iter(*this, size()); }

    struct reader {
        reader(buffer &b) : _b{b}, _pos{0} {}
        reader(const reader &r) : _b{r._b}, _pos{r._pos} {}
        ~reader() {}

        template <typename T> T read(Endian e = sys_endianess);

        template <typename T> bool has_next() {
            return _pos + sizeof(T) < _b._size;
        }

        u8 *read_raw(size_t size);
        void seek(u32 position) { _pos = position; }
        u32 position() { return _pos; }

      private:
        const buffer &_b;
        u32 _pos;
    };

    buffer::reader readinator() { return buffer::reader(*this); }

    /* map in file in buffer, clears buffer beforehand */
    void map_file(const char *file);

    friend std::ostream &operator<<(std::ostream &out, const buffer &b) {
        std::string s{reinterpret_cast<const char *>(b._internal), b._size};
        out << s;
        // out.write(b._internal, b._size);
        // out.write(reinterpret_cast<const char *>(b._internal), b._size);
        return out;
    }

  private:
    void grow(unsigned min_size);

    u8 *_internal;
    unsigned _capacity;
    unsigned _size;
};