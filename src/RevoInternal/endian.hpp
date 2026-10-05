#pragma once

#include <cstdint>
#include <cstring>
#if defined(_MSC_VER)
    #include <cstdlib>
#endif

namespace revointernal {

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define REVO_BIG_ENDIAN 1
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define REVO_LITTLE_ENDIAN 1
#elif defined(_WIN32) || defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
    #define REVO_LITTLE_ENDIAN 1
#else
    #error "Unknown endianness"
#endif

inline uint16_t bswap16(uint16_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(value);
#elif defined(_MSC_VER)
    return _byteswap_ushort(value);
#else
    return (value >> 8) | (value << 8);
#endif
}

inline uint32_t bswap32(uint32_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(value);
#elif defined(_MSC_VER)
    return _byteswap_ulong(value);
#else
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
#endif
}

template<typename T>
inline T readBE(const uint8_t* ptr);

template<>
inline uint16_t readBE<uint16_t>(const uint8_t* ptr) {
    uint16_t value;
    std::memcpy(&value, ptr, sizeof(uint16_t));
#ifdef REVO_LITTLE_ENDIAN
    return bswap16(value);
#else
    return value;
#endif
}

template<>
inline uint32_t readBE<uint32_t>(const uint8_t* ptr) {
    uint32_t value;
    std::memcpy(&value, ptr, sizeof(uint32_t));
#ifdef REVO_LITTLE_ENDIAN
    return bswap32(value);
#else
    return value;
#endif
}

template<>
inline uint8_t readBE<uint8_t>(const uint8_t* ptr) {
    return *ptr;
}

template<>
inline int16_t readBE<int16_t>(const uint8_t* ptr) {
    uint16_t value;
    std::memcpy(&value, ptr, sizeof(uint16_t));
#ifdef REVO_LITTLE_ENDIAN
    return static_cast<int16_t>(bswap16(value));
#else
    return static_cast<int16_t>(value);
#endif
}

inline uint16_t readBE16(const uint8_t* ptr) {
    return readBE<uint16_t>(ptr);
}

inline uint32_t readBE32(const uint8_t* ptr) {
    return readBE<uint32_t>(ptr);
}

template<typename T>
inline void writeBE(uint8_t* ptr, T value);

template<>
inline void writeBE<uint16_t>(uint8_t* ptr, uint16_t value) {
#ifdef REVO_LITTLE_ENDIAN
    value = bswap16(value);
#endif
    std::memcpy(ptr, &value, sizeof(uint16_t));
}

template<>
inline void writeBE<uint32_t>(uint8_t* ptr, uint32_t value) {
#ifdef REVO_LITTLE_ENDIAN
    value = bswap32(value);
#endif
    std::memcpy(ptr, &value, sizeof(uint32_t));
}

template<>
inline void writeBE<uint8_t>(uint8_t* ptr, uint8_t value) {
    *ptr = value;
}

inline void writeBE16(uint8_t* ptr, uint16_t value) {
    writeBE<uint16_t>(ptr, value);
}

inline void writeBE32(uint8_t* ptr, uint32_t value) {
    writeBE<uint32_t>(ptr, value);
}

}
