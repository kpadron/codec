#include <arpa/inet.h>

#include "crc.h"
#include "hash.h"

static buffer_t hash_wrapper(const char* path)
{
    const uint32_t crc = htonl(crc32_filepath(path));
    return buffer_copy(&crc, sizeof(crc));
}

int main(int argc, char** argv)
{
    return hash_main(argc, argv, "CRC-32", hash_wrapper);
}
