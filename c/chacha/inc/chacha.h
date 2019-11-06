#pragma once
#include "utility.h"

typedef struct chacha_ctx
{
    uint32_t state[16];
    uint32_t stream[16];
    size_t index;
} chacha_ctx;

void chacha_init(chacha_ctx* ctx, const uint8_t key[32]);
void chacha_wipe(chacha_ctx* ctx);
void chacha_start(chacha_ctx* ctx, const uint8_t nonce[16]);
void chacha_start_block(chacha_ctx* ctx, uint64_t nonce, uint64_t block);
void chacha_start_offset(chacha_ctx* ctx, uint64_t nonce, uint64_t offset);
void chacha_update(chacha_ctx* ctx, const void* input, void* output, size_t size);
void chacha_crypt(const uint8_t key[32], const uint8_t nonce[16], const void* input, void* output, size_t size);
void chacha_crypt_block(const uint8_t key[32], uint64_t nonce, uint64_t block, const void* input, void* output, size_t size);
void chacha_crypt_offset(const uint8_t key[32], uint64_t nonce, uint64_t offset, const void* input, void* output, size_t size);

// Encode data using chacha codec and return a buffer
void chacha_encode_buffer(buffer_t buffer, const uint8_t key[32], const uint8_t nonce[16]);

// Decode data using chacha codec and return a buffer
void chacha_decode_buffer(buffer_t buffer, const uint8_t key[32], const uint8_t nonce[16]);

// Encode file using the chacha codec
void chacha_encode_filepath(const char* inpath, const char* outpath);

// Decode file using the chacha codec
void chacha_decode_filepath(const char* inpath, const char* outpath);
