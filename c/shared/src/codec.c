#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "codec.h"
#include "utility.h"

int codec_main(int argc, char** argv, const char* codec_name,
    const char* codec_extension, file_encoder_t encode_func,
    file_decoder_t decode_func)
{
    if (argc < 2) return EXIT_FAILURE;

    const bool decode = !strcmp(argv[1], "-d");
    const size_t extension_size = strlen(codec_extension);

    for (int i = 1 + decode; i < argc; i++)
    {
        const char* const path = argv[i];
        if (!filepath_isfile(path)) continue;

        double diff = 0.0;
        const size_t path_size = strlen(path);
        buffer_t outpath_buffer = buffer_copy(path, path_size);

        if (!decode)
        {
            if (!string_endswith(path, codec_extension))
            {
                const size_t outpath_size = path_size + extension_size;
                buffer_resize(&outpath_buffer, outpath_size + 1);
                strcpy((char*) outpath_buffer.data + path_size, codec_extension);

                diff = wtime();
                encode_func(path, (const char*) outpath_buffer.data);
                diff = wtime() - diff;
            }
            else
            {
                printf("%s error: attempting to encode already encoded file\n", codec_name);
                return EXIT_FAILURE;
            }
        }
        else
        {
            if (string_endswith(path, codec_extension))
            {
                const size_t outpath_size = path_size - extension_size;
                buffer_resize(&outpath_buffer, outpath_size + 1);
                outpath_buffer.data[outpath_size] = '\0';

                diff = wtime();
                decode_func(path, (const char*) outpath_buffer.data);
                diff = wtime() - diff;
            }
            else
            {
                printf("%s error: attempting to decode non '%s' file\n", codec_name, codec_extension);
                return EXIT_FAILURE;
            }
        }

        const size_t old_size = filepath_getsize(path);
        const size_t new_size = filepath_getsize((const char*) outpath_buffer.data);
        const size_t max_size = old_size > new_size ? old_size : new_size;

        printf("%s %zu B -> <%s> -> %s %zu B [%.3f s (%.1f B/s)]\n",
            path, old_size, codec_name, (const char*) outpath_buffer.data,
            new_size, diff, (double) max_size / diff);

        buffer_dealloc(&outpath_buffer);
    }

    return EXIT_SUCCESS;
}
