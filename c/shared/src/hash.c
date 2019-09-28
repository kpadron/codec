#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include "hash.h"

int hash_main(int argc, char** argv, const char* hash_name, file_hasher_t hash_func)
{
    if (argc < 2) return EXIT_FAILURE;

    for (int i = 1; i < argc; i++)
    {
        const char* const path = argv[i];
        double diff = wtime();

        buffer_t hash = hash_func(path);

        diff = wtime() - diff;
        const size_t size = filepath_getsize(path);
        buffer_t hash_str = hex_buffer(hash);

        printf("%s %zu B : <%s> %s [%.2f s (%.2f B/s)]\n",
            path, size, hash_name, (const char*) hash_str.data,
            diff, (double) size / diff);

        dealloc_buffer(&hash_str);
        dealloc_buffer(&hash);
    }

    return EXIT_SUCCESS;
}
