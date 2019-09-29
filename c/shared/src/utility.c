#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>

#include <sys/stat.h>

#include "utility.h"

void buffer_init(buffer_t* buffer)
{
    if (!buffer) return;
    *buffer = UTIL_EMPTY_BUFFER;
}

buffer_t buffer_alloc(size_t size)
{
    const size_t aligned_size = align_up2(size);
    buffer_t buffer = { .data = (uint8_t*) UTIL_ALLOC(aligned_size), .size = size, .capacity = aligned_size};
    assert(buffer.data != NULL);

    if (!buffer.data)
    {
        buffer = UTIL_EMPTY_BUFFER;
    }
    else
    {
        memset(buffer.data, 0, buffer.size);
    }

    return buffer;
}

buffer_t buffer_copy(const void* data, size_t size)
{
    if (!data || !size) return UTIL_EMPTY_BUFFER;

    buffer_t buffer = buffer_alloc(size);
    memcpy(buffer.data, data, size);

    return buffer;
}

void buffer_reserve(buffer_t* buffer, size_t capacity)
{
    if (!buffer || capacity <= buffer->capacity) return;

    const size_t new_capacity = align_up2(capacity);
    uint8_t* const new_data = (uint8_t*) UTIL_ALLOC(new_capacity);
    assert(new_data != NULL);

    memcpy(new_data, buffer->data, buffer->size);

    buffer->data = new_data;
    buffer->capacity = new_capacity;
    UTIL_DEALLOC(buffer->data);
}

void buffer_shrink(buffer_t* buffer)
{
    if (!buffer || buffer->size >= (buffer->capacity / 4)) return;

    const size_t new_capacity = align_up2(buffer->size);
    uint8_t* const new_data = (uint8_t*) UTIL_ALLOC(new_capacity);
    assert(new_data != NULL);

    memcpy(new_data, buffer->data, buffer->size);

    buffer->data = new_data;
    buffer->capacity = new_capacity;
    UTIL_DEALLOC(buffer->data);
}

void buffer_resize(buffer_t* buffer, size_t new_size)
{
    if (!buffer || buffer->size == new_size) return;
    if (!new_size) return buffer_dealloc(buffer);

    const size_t old_size = buffer->size;

    // Growing the buffer
    if (new_size > old_size)
    {
        buffer_reserve(buffer, new_size);
        memset(buffer->data + old_size, 0, new_size - old_size);
        buffer->size = new_size;
    }
    // Shrinking the buffer
    else
    {
        buffer->size = new_size;
        buffer_shrink(buffer);
    }
}

void buffer_append(buffer_t* buffer, const void* data, size_t size)
{
    if (!buffer || !data || !size) return;

    const size_t old_size = buffer->size;
    const size_t new_size = old_size + size;
    buffer_reserve(buffer, new_size);
    memcpy(buffer->data + old_size, data, size);
    buffer->size = new_size;
}

void buffer_dealloc(buffer_t* buffer)
{
    if (!buffer) return;

    UTIL_DEALLOC(buffer->data);
    buffer_init(buffer);
}

buffer_t buffer_hex(const buffer_t buffer)
{
    static const uint8_t nibbles[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    };

    buffer_t hex = buffer_alloc(buffer.size * 2 + 1);

    for (size_t i = 0; i < buffer.size; i++)
    {
        const uint8_t upper = (buffer.data[i] & 0xF0) >> 4;
        const uint8_t lower = buffer.data[i] & 0x0F;

        hex.data[i * 2] = nibbles[upper];
        hex.data[i * 2 + 1] = nibbles[lower];
    }

    return hex;
}

bool filepath_isfile(const char* path)
{
    struct stat s = { 0 };
    if (stat(path, &s)) return false;
    return (bool) S_ISREG(s.st_mode);
}

size_t filepath_getsize(const char* path)
{
    struct stat s = { 0 };
    if (stat(path, &s)) return 0;
    return (size_t) s.st_size;
}

double wtime(void)
{
    return (double) clock() / (double) CLOCKS_PER_SEC;
}

bool string_endswith(const char* str, const char* key)
{
    const size_t str_size = strlen(str);
    const size_t key_size = strlen(key);

    if (str_size < key_size) return false;
    const size_t cmp_start = str_size - key_size;

    return !memcmp(str + cmp_start, key, key_size);
}

size_t align_up2(size_t x)
{
    --x;
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    if (sizeof(x) > 4) x |= (x >> 32);

    return ++x;
}
