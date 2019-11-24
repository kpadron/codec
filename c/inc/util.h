#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define U8_C(x) ((u8)(x ## U))
#define U16_C(x) ((u16)(x ## U))
#define U32_C(x) ((u32)(x ## UL))
#define U64_C(x) ((u64)(x ## ULL))

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

#define OFFSET_CPTR(data, offset) ((const u8*)(data) + (offset))
#define OFFSET_PTR(data, offset) ((u8*)(data) + (offset))

static inline u8 rand8(void)
{
    return (u8) rand();
}

static inline u16 rand16(void)
{
    return (u16) rand() | ((u16) rand() << 8);
}

static inline u32 rand32(void)
{
    return (u32) rand16() | ((u32) rand16() << 16);
}

static inline u64 rand64(void)
{
    return (u64) rand32() | ((u64) rand32() << 32);
}

static inline u8 rotl8(u8 value, size_t count)
{
    count &= 7;
    return (value << count) | (value >> (8 - count));
}

static inline u16 rotl16(u16 value, size_t count)
{
    count &= 15;
    return (value << count) | (value >> (16 - count));
}

static inline u32 rotl32(u32 value, size_t count)
{
    count &= 31;
    return (value << count) | (value >> (32 - count));
}

static inline u64 rotl64(u64 value, size_t count)
{
    count &= 63;
    return (value << count) | (value >> (64 - count));
}

static inline u8 rotr8(u8 value, size_t count)
{
    count &= 7;
    return (value >> count) | (value << (8 - count));
}

static inline u16 rotr16(u16 value, size_t count)
{
    count &= 15;
    return (value >> count) | (value << (16 - count));
}

static inline u32 rotr32(u32 value, size_t count)
{
    count &= 31;
    return (value >> count) | (value << (32 - count));
}

static inline u64 rotr64(u64 value, size_t count)
{
    count &= 63;
    return (value >> count) | (value << (64 - count));
}

static inline u16 bswap16(u16 value)
{
    #if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(value);

    #elif defined(_MSC_VER)
    return _byteswap_ushort(value);

    #else
    return ((value & U16_C(0xFF00)) >> 8) |
           ((value & U16_C(0x00FF)) << 8);

    #endif
}

static inline u32 bswap32(u32 value)
{
    #if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(value);

    #elif defined(_MSC_VER)
    return _byteswap_ulong(value);

    #else
    return ((value & U32_C(0xFF000000)) >> 24) |
           ((value & U32_C(0x00FF0000)) >>  8) |
           ((value & U32_C(0x0000FF00)) <<  8) |
           ((value & U32_C(0x000000FF)) << 24);

    #endif
}

static inline u64 bswap64(u64 value)
{
    #if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(value);

    #elif defined(_MSC_VER)
    return _byteswap_uint64(value);

    #else
    return ((value & U64_C(0xFF00000000000000)) >> 56) |
           ((value & U64_C(0x00FF000000000000)) >> 40) |
           ((value & U64_C(0x0000FF0000000000)) >> 24) |
           ((value & U64_C(0x000000FF00000000)) >>  8) |
           ((value & U64_C(0x00000000FF000000)) <<  8) |
           ((value & U64_C(0x0000000000FF0000)) << 24) |
           ((value & U64_C(0x000000000000FF00)) << 40) |
           ((value & U64_C(0x00000000000000FF)) << 56);

    #endif
}

static inline u8 load8(const void* data)
{
    return *((const u8*) data);
}

static inline u16 load16(const void* data)
{
    u16 value;
    memcpy(&value, data, sizeof(value));
    return value;
}

static inline u32 load32(const void* data)
{
    u32 value;
    memcpy(&value, data, sizeof(value));
    return value;
}

static inline u64 load64(const void* data)
{
    u64 value;
    memcpy(&value, data, sizeof(value));
    return value;
}

static inline void store8(void* data, u8 value)
{
    *((u8*) data) = value;
}

static inline void store16(void* data, u16 value)
{
    memcpy(data, &value, sizeof(value));
}

static inline void store32(void* data, u32 value)
{
    memcpy(data, &value, sizeof(value));
}

static inline void store64(void* data, u64 value)
{
    memcpy(data, &value, sizeof(value));
}

static inline u16 load16_le(const void* data)
{
    u16 value = load16(data);

    #if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    value = bswap16(value);
    #endif

    return value;
}

static inline u32 load32_le(const void* data)
{
    u32 value = load32(data);

    #if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    value = bswap32(value);
    #endif

    return value;
}

static inline u64 load64_le(const void* data)
{
    u64 value = load64(data);

    #if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    value = bswap64(value);
    #endif

    return value;
}

static inline void store16_le(void* data, u16 value)
{
    #if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    value = bswap16(value);
    #endif

    store16(data, value);
}

static inline void store32_le(void* data, u32 value)
{
    #if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    value = bswap32(value);
    #endif

    store32(data, value);
}

static inline void store64_le(void* data, u64 value)
{
    #if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    value = bswap64(value);
    #endif

    store64(data, value);
}

static inline u16 load16_be(const void* data)
{
    u16 value = load16(data);

    #if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    value = bswap16(value);
    #endif

    return value;
}

static inline u32 load32_be(const void* data)
{
    u32 value = load32(data);

    #if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    value = bswap32(value);
    #endif

    return value;
}

static inline u64 load64_be(const void* data)
{
    u64 value = load64(data);

    #if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    value = bswap64(value);
    #endif

    return value;
}

static inline void store16_be(void* data, u16 value)
{
    #if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    value = bswap16(value);
    #endif

    store16(data, value);
}

static inline void store32_be(void* data, u32 value)
{
    #if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    value = bswap32(value);
    #endif

    store32(data, value);
}

static inline void store64_be(void* data, u64 value)
{
    #if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    value = bswap64(value);
    #endif

    store64(data, value);
}

// Securely zero out memory
static inline void memwipe(void* data, size_t size)
{
    volatile u8* p = (volatile u8*) data;

    while (size--)
    {
        *p++ = 0;
    }
}

// Xor blocks of memory storing the result in dst
static inline void memxor(
    void* dst,
    const void* src1,
    const void* src2,
    size_t size)
{
    u8* x = (u8*) dst;
    const u8* y = (const u8*) src1;
    const u8* z = (const u8*) src2;

    // Hoping the compiler chooses SIMD instructions (use -O3)
    while (size--)
    {
        *x++ = *y++ ^ *z++;
    }
}

// Fills memory with pseudo-random bytes (not cryptographically secure).
// can be seeded with srand().
static inline void memrand(void* data, size_t size)
{
    u8* p = (u8*) data;

    while (size >= 16)
    {
        u8 buffer[16];
        u8* q = buffer;

        store32_le(buffer + 0, rand32());
        store32_le(buffer + 4, rand32());
        store32_le(buffer + 8, rand32());
        store32_le(buffer + 12, rand32());

        for (size_t i = 0; i < 16; i++)
        {
            *p++ = *q++;
        }

        size -= 16;
    }

    while (size >= 4)
    {
        u32 r = rand32();
        u8* q = (u8*) &r;

        for (size_t i = 0; i < 4; i++)
        {
            *p++ = *q++;
        }

        size -= 4;
    }

    while (size--)
    {
        *p++ = rand8();
    }
}
