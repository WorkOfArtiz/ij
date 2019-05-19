#include "util.hpp"
#include "buffer.hpp"
#include "logger.hpp"
#include <cstdlib>
#include <cstring>
#include <new> // for runtime_error, bad_alloc

/*******************************************************************************
 * Implementations of the append(type), since they're used in functions
 * below they have to be declared earlier
 ******************************************************************************/
template <> void buffer::append<char>(char c, Endian) {
    raw_append(reinterpret_cast<u8 *>(&c), sizeof(char));
}

template <> void buffer::append<const char *>(const char *c, Endian) {
    raw_append(reinterpret_cast<const u8 *>(c), strlen(c));
}

template <> void buffer::append<const buffer &>(const buffer &b, Endian) {
    raw_append(b._internal, b._size);
}

template <> void buffer::append<u8>(u8 v, Endian) {
    raw_append(&v, sizeof(u8));
}

template <> void buffer::append<i8>(i8 v, Endian) {
    raw_append(reinterpret_cast<u8 *>(&v), sizeof(i8));
}

template <> void buffer::append<u16>(u16 v, Endian e) {
    if (e != sys_endianess)
        v = swap_endianess(v);

    buffer::raw_append(reinterpret_cast<u8 *>(&v), sizeof(u16));
}

template <> void buffer::append<i16>(i16 v, Endian e) {
    append(static_cast<u16>(v), e);
}

template <> void buffer::append<u32>(u32 v, Endian e) {
    if (e != sys_endianess)
        v = swap_endianess(v);

    buffer::raw_append(reinterpret_cast<u8 *>(&v), sizeof(u32));
}

template <> void buffer::append<i32>(i32 v, Endian e) {
    append(static_cast<u32>(v), e);
}

/*
 * Write implementations
 */
template <> void buffer::write<u8>(u8 c, u32 offset, Endian) {
    if (offset < _size)
        throw std::runtime_error{"Out of bounds"};

    _internal[offset] = c;
}

template <> void buffer::write<i8>(i8 c, u32 offset, Endian e) {
    write<u8>(*reinterpret_cast<u8 *>(&c), offset, e);
}

template <> void buffer::write<u16>(u16 v, u32 offset, Endian e) {
    union {
        u16 value;
        u8 bytes[sizeof(u16)];
    } value_bytes;

    if (e != sys_endianess)
        value_bytes.value = swap_endianess(v);
    else
        value_bytes.value = v;

    if (offset + 1 > _size)
        throw std::runtime_error{"Out of bounds"};

    _internal[offset + 0] = value_bytes.bytes[0];
    _internal[offset + 1] = value_bytes.bytes[1];
}

template <> void buffer::write<i16>(i16 v, u32 offset, Endian e) {
    write(static_cast<u16>(v), offset, e);
}

template <> void buffer::write<u32>(u32 v, u32 offset, Endian e) {
    union {
        u32 value;
        u8 bytes[sizeof(u32)];
    } value_bytes;

    if (e != sys_endianess)
        value_bytes.value = swap_endianess(v);
    else
        value_bytes.value = v;

    if (offset + 3 > _size)
        throw std::runtime_error{"Out of bounds"};

    _internal[offset + 0] = value_bytes.bytes[0];
    _internal[offset + 1] = value_bytes.bytes[1];
    _internal[offset + 2] = value_bytes.bytes[2];
    _internal[offset + 3] = value_bytes.bytes[3];
}

template <> void buffer::write<i32>(i32 v, u32 offset, Endian e) {
    write(static_cast<u32>(v), offset, e);
}

/*******************************************************************************
 * buffer::reader::read implementations
 ******************************************************************************/
template <> u8 buffer::reader::read<u8>(Endian) {
    if (_pos >= _b._size)
        throw std::runtime_error{"buffer overflow in reader"};

    return _b[_pos++];
}

template <> i8 buffer::reader::read<i8>(Endian e) {
    return static_cast<i8>(this->read<u8>(e));
}

template <> u16 buffer::reader::read(Endian e) {
    if (_pos + 1 >= _b._size)
        throw std::runtime_error{"tried to read past end of buffer"};

    u16 x = *reinterpret_cast<const u16 *>(&_b[_pos]);
    _pos += 2;

    if (e != sys_endianess)
        x = swap_endianess(x);

    return x;
}

template <> i16 buffer::reader::read(Endian e) {
    return static_cast<i16>(this->read<u16>(e));
}

template <> u32 buffer::reader::read(Endian e) {
    if (_pos + 4 > _b._size)
        throw std::runtime_error{"tried to read past end of buffer"};

    u32 x = *reinterpret_cast<const u32 *>(&_b[_pos]);
    _pos += 4;

    if (e != sys_endianess)
        x = swap_endianess(x);

    return x;
}

template <> i32 buffer::reader::read(Endian e) {
    return static_cast<i32>(this->read<u32>(e));
}

/*******************************************************************************
 * Buffer class implementation
 ******************************************************************************/
buffer::buffer(unsigned capacity) : _capacity{capacity}, _size{0} {
    _internal = reinterpret_cast<u8 *>(malloc(capacity));

    if (!_internal)
        throw std::bad_alloc();
}

buffer::buffer(const buffer &b) {
    std::cout << "I shouldn't have been called, damn it" << std::endl;
    _internal = static_cast<u8 *>(malloc(b._capacity));
    _size = b._size;
    _capacity = b._capacity;
    memcpy(_internal, b._internal, _size);
}

buffer::~buffer() {
    if (_internal != nullptr)
        free(_internal);
}

void buffer::operator=(buffer &&b) {
    _internal = b._internal;
    _size = b._size;
    _capacity = b._capacity;

    b._internal = nullptr;
    b._size = 0;
    b._capacity = 0;
}

const u8 &buffer::operator[](unsigned index) const {
    if (index >= size())
        throw std::runtime_error("Buffer overflow");

    return this->_internal[index];
}

void buffer::raw_append(const u8 *raw, unsigned size) {
    if (_size + size > _capacity)
        grow(_size + size);

    for (unsigned i = 0; i < size; i++) {
        // std::cout << "Appending: " << raw[i] << " (" << (int) raw[i] << ")"
        // << std::endl;
        _internal[_size + i] = raw[i];
    }
    _size += size;
}

buffer buffer::escape() {
    buffer escaped;
    auto hex = [](u8 i) -> char { return (i < 10) ? '0' + i : 'a' + i - 10; };

    for (u8 byte : *this) {
        if (byte == '\\') {
            escaped.append('\\');
            escaped.append('\\');
            continue;
        }

        if (byte >= ' ' && byte <= '~') {
            escaped.append(static_cast<char>(byte));
            continue;
        }

        escaped.append('\\');
        switch (byte) {
        case '\0':
            escaped.append('0');
            break;
        case '\t':
            escaped.append('t');
            break;
        case '\n':
            escaped.append('n');
            break;
        case '\r':
            escaped.append('r');
            break;
        case '\a':
            escaped.append('a');
            break;
        case '\033':
            escaped.append('e');
            break;
        default:
            escaped.append('x');
            escaped.append(hex(byte >> 4));
            escaped.append(hex(byte & 0xf));
        }
    }

    escaped.append('\0');
    return escaped;
}

void buffer::grow(unsigned new_size) {
    unsigned new_cap = _capacity;

    if (_capacity >= new_size)
        return;

    while (new_cap < new_size)
        new_cap *= 2;

    log.info("Growing buffer from %d to %d", _capacity, new_cap);

    void *internal_new = realloc(_internal, new_cap);
    if (!internal_new)
        throw std::bad_alloc();

    _internal = reinterpret_cast<u8 *>(internal_new);
    _capacity = new_cap;
}

void buffer::map_file(const char *file) {
    clear();

    FILE *input_file = fopen(file, "rb");
    if (input_file == nullptr)
        throw std::runtime_error{sprint("File '%s' doesn't exist", file)};

    u8 contents[1024];
    size_t bytes_read;

    while ((bytes_read = fread(contents, 1, 1024, input_file)) != 0) {
        raw_append(contents, bytes_read);
    }
    fclose(input_file);
}