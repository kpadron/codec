#include <stdlib.h>
#include <stdio.h>

#include "hash.h"

int hash_main(int argc, char** argv, file_hasher_t hasher)
{
    uint32_t x = 0x7FFFFFFF;
    buffer_t str = string_buffer(copy_buffer(&x, sizeof(x)));
    printf("Hello, World!: %s\n", (const char*)str.data);
    return 0;
}

int main(int argc, char** argv)
{
    return hash_main(argc, argv, NULL);
}
