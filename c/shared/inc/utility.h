#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define UTIL_ALLOC malloc
#define UTIL_DEALLOC free

typedef struct _buffer_t
{
    uint8_t* data;
    size_t size;
    size_t capacity;
} buffer_t;

#define UTIL_EMPTY_BUFFER (const buffer_t){ .data = NULL, .size = 0, .capacity = 0 }

void buffer_init(buffer_t* buffer);
buffer_t buffer_alloc(size_t size);
buffer_t buffer_copy(const void* data, size_t size);
void buffer_reserve(buffer_t* buffer, size_t capacity);
void buffer_shrink(buffer_t* buffer);
void buffer_resize(buffer_t* buffer, size_t new_size);
void buffer_append(buffer_t* buffer, const void* data, size_t size);
void buffer_dealloc(buffer_t* buffer);
buffer_t buffer_hex(const buffer_t buffer);

bool filepath_isfile(const char* path);
size_t filepath_getsize(const char* path);


double wtime(void);

bool string_endswith(const char* str, const char* key);

size_t rev_bits(size_t x, size_t width);
size_t align_up2(size_t x);
