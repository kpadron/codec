#include <stdio.h>

#include "hash.h"

int hash_main(int argc, char** argv, const char* hash_name, file_hasher_t hash_func)
{
    if (argc < 2) return EXIT_FAILURE;

    for (int i = 1; i < argc; i++)
    {
        const char* const path = argv[i];
        if (!filepath_isfile(path)) continue;

        double diff = wtime();
        buffer_t hash = hash_func(path);
        diff = wtime() - diff;

        const size_t size = filepath_getsize(path);
        buffer_t hash_str = buffer_hex(hash);

        printf("%s %zu B : <%s> %s [%.3f s (%.1f B/s)]\n",
            path, size, hash_name, (const char*) hash_str.data,
            diff, (double) size / diff);

        buffer_dealloc(&hash_str);
        buffer_dealloc(&hash);
    }

    return EXIT_SUCCESS;
}
