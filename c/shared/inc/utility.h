#pragma once
#include <stdint.h>
#include <stdlib.h>

#define UTIL_ALLOC malloc
#define UTIL_REALLOC realloc
#define UTIL_DEALLOC free

typedef struct
{
    uint8_t* data;
    size_t size;
} buffer_t;

static const buffer_t UTIL_EMPTY_BUFFER = { NULL, 0 };

void init_buffer(buffer_t* buffer);
buffer_t alloc_buffer(size_t size);
buffer_t copy_buffer(const void* data, size_t size);
void resize_buffer(buffer_t* buffer, size_t new_size);
void dealloc_buffer(buffer_t* buffer);
buffer_t hex_buffer(buffer_t buffer);

size_t filepath_getsize(const char* path);

double wtime(void);

size_t rev_bits(size_t x, size_t width);
