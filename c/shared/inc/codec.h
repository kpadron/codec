#pragma once

typedef void (*file_encoder_t)(const char*, const char*);
typedef void (*file_decoder_t)(const char*, const char*);

int codec_main(int argc, char** argv, const char* codec_name,
    const char* codec_extension, file_encoder_t encode_func,
    file_decoder_t decode_func);
