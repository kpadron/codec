#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "chacha.h"

#define BUFFER_SIZE (1 << 16)

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

static bool copy_file(const char* inpath, const char* outpath)
{
    FILE* infile = fopen(inpath, "rb");
    FILE* outfile = fopen(outpath, "wb");
    bool success = true;

    // Handle file or allocation failure
    if (infile == NULL || outfile == NULL)
    {
        goto error;
    }

    // Disable I/O buffering
    setbuf(infile, NULL);
    setbuf(outfile, NULL);

    // Copy file blocks
    for (;;)
    {
        u8 buffer[BUFFER_SIZE];

        // Read into buffer then write buffer
        const size_t read_size = fread(buffer, 1, BUFFER_SIZE, infile);
        const size_t write_size = fwrite(buffer, 1, read_size, outfile);

        // Check for EOF and errors
        if (read_size != BUFFER_SIZE || read_size != write_size)
        {
            if (ferror(infile) || ferror(outfile))
            {
                goto error;
            }

            break;
        }
    }

    goto exit;

error:
    fprintf(stderr, "[copy_file] copy '%s' to '%s' failed: %s\n",
            inpath, outpath, strerror(errno));

    success = false;

exit:
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

    return success;
}

bool crypt_file_parallel(const char* path, const u8 key[CHACHA_KEY_SIZE])
{
    size_t size = filesize(path);
    FILE* file = fopen(path, "rb+");
    const size_t full_blocks = size / BUFFER_SIZE;
    const size_t partial_size = size % BUFFER_SIZE;
    bool success = true;

    // Handle file or allocation failure
    if (file == NULL || size == 0)
    {
        goto error;
    }

    // Disable I/O buffering
    setbuf(file, NULL);

    // Process full blocks of file data (using parallel threads if possible)
    #pragma omp parallel for default(shared) schedule(nonmonotonic:dynamic)
    for (size_t i = 0; i < full_blocks; i++)
    {
        const u64 offset = i * BUFFER_SIZE;
        u8 buffer[BUFFER_SIZE];

        // Seekt to correct file position and read into buffer
        #pragma omp critical(file)
        {
            if (success)
            {
                fseek(file, (long int) offset, SEEK_SET);
                size_t read_size = fread(buffer, 1, BUFFER_SIZE, file);

                if (read_size != BUFFER_SIZE)
                {
                    success = false;
                }
            }
        }

        // Uses a nonce with all zero bytes. NOTE: reusing a nonce value with the same
        // key when encrypting two different plaintexts will void the security of the cipher.
        // This example always uses a nonce of all zero bytes and is only for testing purposes.

        // Crypt local file data
        chacha_crypt_offset(key, 0, offset, buffer, buffer, BUFFER_SIZE);

        // Seek to correct file position and write crypted data
        #pragma omp critical(file)
        {
            if (success)
            {
                fseek(file, (long int) offset, SEEK_SET);
                size_t write_size = fwrite(buffer, 1, BUFFER_SIZE, file);

                if (write_size != BUFFER_SIZE)
                {
                    success = false;
                }
            }
        }
    }

    // Handle last partial block of file data
    if (partial_size > 0)
    {
        const size_t offset = full_blocks * BUFFER_SIZE;
        u8 buffer[BUFFER_SIZE];

        fseek(file, (long int) offset, SEEK_SET);
        size_t read_size = fread(buffer, 1, partial_size, file);

        if (read_size != partial_size)
        {
            goto error;
        }

        chacha_crypt_offset(key, 0, offset, buffer, buffer, partial_size);

        fseek(file, (long int) offset, SEEK_SET);
        size_t write_size = fwrite(buffer, 1, partial_size, file);

        if (write_size != read_size)
        {
            goto error;
        }
    }

    goto exit;

error:
    fprintf(stderr, "[crypt_file_parallel] crypting '%s' failed: %s\n",
            path, strerror(errno));

    success = false;

exit:
    // Close input file
    if (file != NULL)
    {
        fclose(file);
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

    // Copy input file to output path
    if (inpath != outpath && strcmp(inpath, outpath) != 0)
    {
        printf("Copying '%s' to '%s'...", inpath, outpath); fflush(stdout);
        success = copy_file(inpath, outpath);
        printf(success ? " done.\n" : " failed.\n");
    }

    // Crypt file in parallel using threads
    if (success)
    {
        printf("Crypting '%s' using ChaCha cipher...", outpath); fflush(stdout);
        success = crypt_file_parallel(outpath, key_buffer);
        printf(success ? " done.\n" : " failed.\n");
    }

    // Securely wipe key buffer
    memwipe(key_buffer, sizeof(key_buffer));

exit:
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
