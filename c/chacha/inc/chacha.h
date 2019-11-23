#pragma once
#include "utility.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

const size_t CHACHA_ROUNDS = 20;
const size_t CHACHA_KEY_SIZE = 32;

typedef struct chacha_ctx
{
    u32 state[16];
    u32 stream[16];
    size_t index;
} chacha_ctx;

void chacha_init(chacha_ctx* ctx, const u8 key[32]);
void chacha_wipe(chacha_ctx* ctx);
void chacha_start(chacha_ctx* ctx, const u8 nonce[16]);
void chacha_start_block(chacha_ctx* ctx, u64 nonce, u64 block);
void chacha_start_offset(chacha_ctx* ctx, u64 nonce, u64 offset);
void chacha_update(chacha_ctx* ctx, const void* input, void* output, size_t size);
void chacha_crypt(const u8 key[32], const u8 nonce[16], const void* input, void* output, size_t size);
void chacha_crypt_block(const u8 key[32], u64 nonce, u64 block, const void* input, void* output, size_t size);
void chacha_crypt_offset(const u8 key[32], u64 nonce, u64 offset, const void* input, void* output, size_t size);

// Encode data using chacha codec and return a buffer
void chacha_encode_buffer(buffer_t buffer, const u8 key[32], const u8 nonce[16]);

// Decode data using chacha codec and return a buffer
void chacha_decode_buffer(buffer_t buffer, const u8 key[32], const u8 nonce[16]);

// Encode file using the chacha codec
void chacha_encode_filepath(const char* inpath, const char* outpath);

// Decode file using the chacha codec
void chacha_decode_filepath(const char* inpath, const char* outpath);
