#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "utility.h"

const buffer_t DEFAULT_BUFFER = { .data = NULL, .size = 0 };

void init_buffer(buffer_t* buffer)
{
    if (!buffer) return;
    *buffer = DEFAULT_BUFFER;
}

buffer_t alloc_buffer(size_t size)
{
    buffer_t buffer = { .data = (uint8_t*) UTIL_ALLOC(size), .size = size };

    if (!buffer.data)
    {
        buffer = DEFAULT_BUFFER;
    }

    return buffer;
}

buffer_t copy_buffer(const void* data, size_t size)
{
    buffer_t buffer = DEFAULT_BUFFER;
    if (!data) return buffer;

    resize_buffer(&buffer, size);
    memcpy(buffer.data, data, size);

    return buffer;
}

void resize_buffer(buffer_t* buffer, size_t new_size)
{
    if (!buffer || buffer->size == new_size) return;
    if (!new_size) return dealloc_buffer(buffer);

    const size_t old_size = buffer->size;

    buffer->data = (uint8_t*) UTIL_REALLOC(buffer->data, new_size);

    if (!buffer->data)
    {
        init_buffer(buffer);
        return;
    }

    if (old_size < new_size)
    {
        uint8_t* const new_data = buffer->data + old_size;
        const size_t size_delta = new_size - old_size;
        memset(new_data, '\0', size_delta);
    }

    buffer->size = new_size;
}

void dealloc_buffer(buffer_t* buffer)
{
    if (!buffer) return;

    UTIL_DEALLOC(buffer->data);
    init_buffer(buffer);
}

buffer_t string_buffer(buffer_t buffer)
{
    static const uint8_t nibbles[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    };

    buffer_t str = alloc_buffer(buffer.size * 2 + 1);

    for (size_t i = 0; i < buffer.size; i++)
    {
        uint8_t upper = (buffer.data[i] & 0xF0) >> 4;
        uint8_t lower = buffer.data[i] & 0x0F;

        str.data[i * 2] = nibbles[upper];
        str.data[i * 2 + 1] = nibbles[lower];
    }

    return str;
}
