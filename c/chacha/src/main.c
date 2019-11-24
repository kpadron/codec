#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "chacha.h"

#define BUFFER_SIZE (1 << 21)
#define FILE_SIZE_THRESHOLD (1 << 30)

static size_t filesize(const char* filename)
{
    size_t size = 0;
    struct stat sb;

    if (stat(filename, &sb) != -1)
    {
        size = (size_t) sb.st_size;
    }

    return size;
}

static u8* filemap(const char* filename, size_t size)
{
    u8* mapping = NULL;
    int fd = open(filename, O_RDWR);

    if (fd == -1)
    {
        goto error;
    }

    mapping = (u8*) mmap(NULL, size, PROT_READ |
        PROT_WRITE, MAP_SHARED, fd, 0);

    if (mapping != MAP_FAILED)
    {
        goto exit;
    }

error:
    fprintf(stderr, "[filemap] failed to memory map '%s' %zu: %s\n",
        filename, size, strerror(errno));

    mapping = NULL;

exit:
    return mapping;
}

static void fileunmap(void* mapping, size_t size)
{
    if (munmap(mapping, size) != 0)
    {
        fprintf(stderr, "[fileunmap] failed to unmap memory '%p' %zu: %s\n",
            mapping, size, strerror(errno));
    }
}

static bool copy_file(const char* inpath, const char* outpath)
{
    FILE* infile = fopen(inpath, "rb");
    FILE* outfile = fopen(outpath, "wb");
    u8* const buffer = (u8*) malloc(BUFFER_SIZE);
    bool success = false;

    // Handle file or allocation failure
    if (infile == NULL || outfile == NULL || buffer == NULL)
    {
        fprintf(stderr, "[copy_file] copy '%s' to '%s' failed: %s\n",
            inpath, outpath, strerror(errno));

        goto error;
    }

    // Disable I/O buffering
    setbuf(infile, NULL);
    setbuf(outfile, NULL);

    // Copy file blocks
    for (;;)
    {
        // Read into buffer then write buffer
        const size_t read_size = fread(buffer, 1, BUFFER_SIZE, infile);
        const size_t write_size = fwrite(buffer, 1, read_size, outfile);

        // Check for EOF and display errors
        if (read_size != BUFFER_SIZE || read_size != write_size)
        {
            if (!ferror(infile) && !ferror(outfile))
            {
                success = true;
            }
            else
            {
                fprintf(stderr, "[copy_file] copy '%s' to '%s' failed: %s\n",
                    inpath, outpath, strerror(errno));
            }

            break;
        }
    }

error:
    // Close input file
    if (infile != NULL)
    {
        fclose(infile);
    }

    // Close output file
    if (outfile != NULL)
    {
        fclose(outfile);
    }

    // Securely wipe buffer and free memory
    if (buffer != NULL)
    {
        memwipe(buffer, BUFFER_SIZE);
        free(buffer);
    }

    return success;
}

static bool crypt_file_inplace(const char* path, const u8 key[CHACHA_KEY_SIZE])
{
    FILE* file = fopen(path, "rb+");
    u8* const buffer = (u8*) malloc(BUFFER_SIZE);
    bool success = false;
    chacha_ctx ctx;

    // Handle file or allocation failure
    if (file == NULL || buffer == NULL)
    {
        fprintf(stderr, "[crypt_file_inplace] crypting '%s' failed: %s\n",
            path, strerror(errno));

        goto error;
    }

    // Disable I/O buffering
    setbuf(file, NULL);

    // Initialize chacha context with key. Starts the stream counter at 0.
    // Uses a nonce with all zero bytes. NOTE: reusing a nonce value with the same
    // key when encrypting two different plaintexts will void the security of the cipher.
    // This example always uses a nonce of all zero bytes and is only for testing purposes.
    chacha_init(&ctx, key);

    // Crypt blocks from the input file and write to the output file
    for (;;)
    {
        // Read data into buffer
        const size_t read_size = fread(buffer, 1, BUFFER_SIZE, file);

        // Crypt data in buffer
        chacha_update(&ctx, buffer, buffer, read_size);

        // Go back and write crypted buffer data
        fseek(file, -((long) read_size), SEEK_CUR);
        const size_t write_size = fwrite(buffer, 1, read_size, file);

        // Check for EOF and display errors
        if (read_size != BUFFER_SIZE || read_size != write_size)
        {
            if (!ferror(file))
            {
                success = true;
            }
            else
            {
                fprintf(stderr, "[crypt_file_inplace] crypting '%s' failed: %s\n",
                    path, strerror(errno));
            }

            break;
        }
    }

    // Securely teardown the chacha context
    chacha_wipe(&ctx);

error:
    // Close input file
    if (file != NULL)
    {
        fclose(file);
    }

    // Securely wipe buffer and free memory
    if (buffer != NULL)
    {
        memwipe(buffer, BUFFER_SIZE);
        free(buffer);
    }

    return success;
}

#include <omp.h>

#define BLOCK_OFFSET(id, total, size) ((id) * (size) / (total))
#define BLOCK_SIZE(id, total, size) (BLOCK_OFFSET((id) + 1, (total), (size)) - BLOCK_OFFSET((id), (total), (size)))

bool crypt_file_parallel(const char* path, const u8 key[CHACHA_KEY_SIZE])
{
    size_t size = filesize(path);
    u8* fdata = filemap(path, size);
    bool success = true;

    // Handle file or allocation failure
    if (fdata == NULL || size == 0)
    {
        goto error;
    }

    // Determine total parallel threads

    printf("Using %d parallel threads...\n", omp_get_max_threads());

    // Distribute file blocks to parallel threads
    #pragma omp parallel
    {
        // Get current thread id
        const int id = omp_get_thread_num();
        const int total = omp_get_num_threads();

        // Determine file data block for this thread
        const size_t block_offset = BLOCK_OFFSET(id, total, size);
        const size_t block_size = BLOCK_SIZE(id, total, size);
        u8* const block = fdata + block_offset;

        printf("Thread %d/%d %p %zu %zu\n", id, total, block, block_offset, block_size);

        // Crypt thread local file data block
        chacha_crypt_offset(key, 0, block_offset, block, block, block_size);
    }

    goto exit;

error:
    fprintf(stderr, "[crypt_file_parallel] crypting '%s' failed: %s\n",
            path, strerror(errno));

    success = false;

exit:
    if (fdata != NULL)
    {
        fileunmap(fdata, size);
    }

    return success;
}

int main(int argc, char** argv)
{
    u8 key_buffer[CHACHA_KEY_SIZE] = { 0 };
    bool success = true;

    if (argc < 2)
    {
        printf("Usage: %s input_file [password] [output_file]\n", argv[0]);
        goto exit;
    }

    const char* inpath = argv[1];
    char* password = argc > 2 ? argv[2] : NULL;
    const char* outpath = argv[argc < 4 ? 1 : 3];

    // Combine password with empty key buffer
    if (password != NULL)
    {
        size_t password_len = strlen(password);
        memxor(key_buffer, key_buffer, password, MIN(sizeof(key_buffer), password_len));
        memwipe(password, password_len);
    }

    printf("Crypting '%s' to '%s' using ChaCha cipher...\n", inpath, outpath);

    // Copy input file to output path then crypt in-place
    if (inpath != outpath && strcmp(inpath, outpath) != 0)
    {
        success = copy_file(inpath, outpath);
    }

    // Crypt file in parallel if below size threshold
    if (filesize(outpath) <= FILE_SIZE_THRESHOLD)
    {
        success = success && crypt_file_parallel(outpath, key_buffer);
    }
    // Crypt file using normal single threaded I/O
    else
    {

        success = success && crypt_file_inplace(outpath, key_buffer);
    }

    // Securely wipe key buffer
    memwipe(key_buffer, sizeof(key_buffer));

    // Print results
    if (success)
    {
        printf("Crypting successful.\n");
    }
    else
    {
        printf("Crypting failed.\n");
    }

exit:
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
