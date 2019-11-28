#include <stdio.h>
#include <stdbool.h>

#include "crc.h"

static bool crc_file(const char * const restrict path, u32 * const restrict crc)
{
    *crc = crc32_filepath(path);

    if (!*crc)
    {
        return false;
    }

    return true;
}

int main(int argc, char** argv)
{
    u32 crc = 0;
    bool success = true;

    if (argc < 2)
    {
        printf("Usage: %s input_file\n", argv[0]);
        goto exit;
    }

    const char* inpath = argv[1];

    // CRC file
    printf("Hashing '%s' using CRC-32 hash...", inpath); fflush(stdout);
    success = crc_file(inpath, &crc);
    printf(success ? " done. %u\n" : " failed. %u\n", crc);

exit:
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
