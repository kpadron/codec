#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "chacha.h"

#define FILE_BLOCK_SIZE (1 << 20)

#define MIN(x, y) ((x) <= (y) ? (x) : (y))

static void memwipe(void* data, size_t size)
{
    volatile u8* p = (volatile u8*) data;

    while (size--)
    {
        *p++ = 0;
    }
}

static u32 bswap32(u32 value)
{
    #if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(value);

    #elif defined(_MSC_VER)
    return _byteswap_ulong(value);

    #else
    return ((value & 0xFF000000U) >> 24) |
           ((value & 0x00FF0000U) >>  8) |
           ((value & 0x0000FF00U) <<  8) |
           ((value & 0x000000FFU) << 24);

    #endif
}

static u32 load32(const void* data)
{
    u32 value;
    memcpy(&value, data, sizeof(value));
    return value;
}

static u32 load32_le(const void* data)
{
    u32 value = load32(data);

    #if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    value = bswap32(value);
    #endif

    return value;
}

static void store32(void* data, u32 value)
{
    memcpy(data, &value, sizeof(value));
}

static void store32_le(void* data, u32 value)
{
    #if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    value = bswap32(value);
    #endif

    store32(data, value);
}

static u32 rotl32(u32 value, u32 count)
{
    #if defined(_MSC_VER)
    return _rotl(value, (int) count);

    #else
    count &= 31;
    return (value << count) | (value >> (32 - count));

    #endif
}

static void xor_block(void* dst, const void* src1, const void* src2, size_t size)
{
    u8* x = (u8*) dst;
    const u8* y = (const u8*) src1;
    const u8* z = (const u8*) src2;

    // Hoping the compiler chooses SIMD instructions
    while (size--)
    {
        *x++ = *y++ ^ *z++;
    }
}

static const size_t CHACHA_BLOCK_SIZE = 64;

#define CHACHA_SIGMA "expand 32-byte k"
#define CHACHA_TAU "expand 16-byte k"
#if CHACHA_KEY_SIZE == 32
static const char CHACHA_CONSTANT[] = CHACHA_SIGMA;
#else
static const char CHACHA_CONSTANT[] = CHACHA_TAU;
#endif

// Perform the ChaCha quarter round
static void chacha_quarter_round(u32 block[16], size_t a, size_t b, size_t c, size_t d)
{
    // a += b; d ^= a; d <<<= 16;
    block[a] += block[b]; block[d] = rotl32(block[d] ^ block[a], 16);

    // c += d; b ^= c; b <<<= 12;
    block[c] += block[d]; block[b] = rotl32(block[b] ^ block[c], 12);

    // a += b; d ^= a; d <<<= 8;
    block[a] += block[b]; block[d] = rotl32(block[d] ^ block[a], 8);

    // c += d; b ^= c; b <<<= 7;
    block[c] += block[d]; block[b] = rotl32(block[b] ^ block[c], 7);
}

// Increment 128-bit (16-byte) nonce as little-endian counter
static inline void chacha_increment(u32 counter[4])
{
    for (size_t i = 0; i < 4; i++)
    {
        if (++counter[i])
        {
            break;
        }
    }
}

// Generate ChaCha cipher keystream
static void chacha_block(const u32 input[16], u32 output[16])
{
    // Load initial state into output buffer
    memcpy(output, input, CHACHA_BLOCK_SIZE);

    // Perform ChaCha rounds
    for (size_t i = 0; i < CHACHA_ROUNDS; i += 2)
    {
        // Odd round
        chacha_quarter_round(output, 0, 4, 8, 12);
        chacha_quarter_round(output, 1, 5, 9, 13);
        chacha_quarter_round(output, 2, 6, 10, 14);
        chacha_quarter_round(output, 3, 7, 11, 15);

        // Even round
        chacha_quarter_round(output, 0, 5, 10, 15);
        chacha_quarter_round(output, 1, 6, 11, 12);
        chacha_quarter_round(output, 2, 7, 8, 13);
        chacha_quarter_round(output, 3, 4, 9, 14);
    }

    // Encode generated keystream window
    for (size_t i = 0; i < 16; i++)
    {
        store32_le(output + i, output[i] + input[i]);
    }
}

// Xor ChaCha keystream with data block
static void chacha_xor_block(const u8 stream[64], const u8 input[64], u8 output[64])
{
    xor_block(output, input, stream, CHACHA_BLOCK_SIZE);
}

// Set initial state for ChaCha cipher context
void chacha_init(chacha_ctx* ctx, const u8 key[32])
{
    if (ctx == NULL || key == NULL) return;

    // ChaCha constant
    ctx->state[0] = load32_le(CHACHA_CONSTANT + 0);
    ctx->state[1] = load32_le(CHACHA_CONSTANT + 4);
    ctx->state[2] = load32_le(CHACHA_CONSTANT + 8);
    ctx->state[3] = load32_le(CHACHA_CONSTANT + 12);

    // 256-bit (32-byte) or 128-bit (16-byte) key
    ctx->state[4] = load32_le(key + 0);
    ctx->state[5] = load32_le(key + 4);
    ctx->state[6] = load32_le(key + 8);
    ctx->state[7] = load32_le(key + 12);
    ctx->state[8] = load32_le(key + 16 % CHACHA_KEY_SIZE);
    ctx->state[9] = load32_le(key + 20 % CHACHA_KEY_SIZE);
    ctx->state[10] = load32_le(key + 24 % CHACHA_KEY_SIZE);
    ctx->state[11] = load32_le(key + 28 % CHACHA_KEY_SIZE);

    // 128-bit (16-byte) nonce/counter value
    ctx->state[12] = 0;
    ctx->state[13] = 0;
    ctx->state[14] = 0;
    ctx->state[15] = 0;

    // Keystream window index
    ctx->index = CHACHA_BLOCK_SIZE;
}

// Securely erase the ChaCha cipher context
void chacha_wipe(chacha_ctx* ctx)
{
    if (ctx != NULL)
    {
        memwipe(ctx, sizeof(*ctx));
    }
}

// Seek to the relevant block in the keystream based on
// 128-bit (16-byte) nonce
void chacha_seek(chacha_ctx* ctx, const u8 nonce[16])
{
    if (ctx == NULL || nonce == NULL) return;

    ctx->state[12] = load32_le(nonce + 0);
    ctx->state[13] = load32_le(nonce + 4);
    ctx->state[14] = load32_le(nonce + 8);
    ctx->state[15] = load32_le(nonce + 12);

    ctx->index = CHACHA_BLOCK_SIZE;
}

// Seek to the relevant block in the keystream based on
// 64-bit (8-byte) nonce and block counter
void chacha_seek_block(chacha_ctx* ctx, u64 nonce, u64 block)
{
    if (ctx == NULL) return;

    ctx->state[12] = (u32)(block >> 0);
    ctx->state[13] = (u32)(block >> 32);
    ctx->state[14] = (u32)(nonce >> 0);
    ctx->state[15] = (u32)(nonce >> 32);

    ctx->index = CHACHA_BLOCK_SIZE;
}

// Seek to the relevant byte in the keystream based on
// 64-bit (8-byte) nonce and byte offset
void chacha_seek_offset(chacha_ctx* ctx, u64 nonce, u64 offset)
{
    if (ctx == NULL) return;

    const u64 block = offset / CHACHA_BLOCK_SIZE;
    const u64 index = offset % CHACHA_BLOCK_SIZE;

    // Seek to relevant keystream block
    chacha_seek_block(ctx, nonce, block);

    // Seek to relevant keystream byte
    if (index)
    {
        chacha_block(ctx->state, ctx->stream);
        chacha_increment(ctx->state + 12);
        ctx->index = (size_t) index;
    }
}

// Crypt bytes based on ChaCha cipher context
void chacha_update(chacha_ctx* ctx, const void* input, void* output, size_t size)
{
    if (ctx == NULL || input == NULL || output == NULL || size == 0) return;

    const u8* const stream = (const u8*) ctx->stream;
    const u8* in = (const u8*) input;
    u8* out = (u8*) output;

    if (size > 0 && ctx->index < CHACHA_BLOCK_SIZE)
    {
        const size_t msize = MIN(size, CHACHA_BLOCK_SIZE - ctx->index);

        xor_block(out, in, stream + ctx->index, msize);

        in += msize;
        out += msize;
        ctx->index += msize;
        size -= msize;
    }

    while (size >= CHACHA_BLOCK_SIZE)
    {
        chacha_block(ctx->state, ctx->stream);
        chacha_increment(ctx->state + 12);
        xor_block(out, in, stream, CHACHA_BLOCK_SIZE);

        in += CHACHA_BLOCK_SIZE;
        out += CHACHA_BLOCK_SIZE;
        size -= CHACHA_BLOCK_SIZE;
    }

    if (size > 0)
    {
        chacha_block(ctx->state, ctx->stream);
        chacha_increment(ctx->state + 12);
        xor_block(out, in, stream, size);
        ctx->index = size;
    }
}

void chacha_crypt(const u8 key[32], const u8 nonce[16], const void* input, void* output, size_t size)
{
    chacha_ctx ctx;

    if (key == NULL || nonce == NULL || input == NULL || output == NULL || size == 0) return;

    chacha_init(&ctx, key);
    chacha_start(&ctx, nonce);
    chacha_update(&ctx, input, output, size);
    chacha_wipe(&ctx);
}

void chacha_crypt_block(const u8 key[32], u64 nonce, u64 block, const void* input, void* output, size_t size)
{
    chacha_ctx ctx;

    if (key == NULL || input == NULL || output == NULL || size == 0) return;

    chacha_init(&ctx, key);
    chacha_start_block(&ctx, nonce, block);
    chacha_update(&ctx, input, output, size);
    chacha_wipe(&ctx);
}

void chacha_crypt_offset(const u8 key[32], u64 nonce, u64 offset, const void* input, void* output, size_t size)
{
    chacha_ctx ctx;

    if (key == NULL || input == NULL || output == NULL || size == 0) return;

    chacha_init(&ctx, key);
    chacha_start_offset(&ctx, nonce, offset);
    chacha_update(&ctx, input, output, size);
    chacha_wipe(&ctx);
}

void chacha_encode_buffer(buffer_t buffer, const u8 key[32], const u8 nonce[16])
{
    chacha_crypt(key, nonce, buffer.data, buffer.data, buffer.size);
}

void chacha_decode_buffer(buffer_t buffer, const u8 key[32], const u8 nonce[16])
{
    chacha_crypt(key, nonce, buffer.data, buffer.data, buffer.size);
}

void chacha_encode_filepath(const char* inpath, const char* outpath)
{
    FILE* infile = fopen(inpath, "rb");
    FILE* outfile = fopen(outpath, "wb");
    u8* buffer = (u8*) malloc(FILE_BLOCK_SIZE);
    chacha_ctx ctx;

    if (infile == NULL || outfile == NULL || buffer == NULL) goto error;

    chacha_init(&ctx, (const u8[32]){ 0 });
    chacha_start(&ctx, (const u8[16]){ 0 });

    for (;;)
    {
        size_t read_size = fread(buffer, 1, FILE_BLOCK_SIZE, infile);
        size_t write_size = 0;

        chacha_update(&ctx, buffer, buffer, read_size);

        write_size = fwrite(buffer, 1, read_size, outfile);

        if (read_size != FILE_BLOCK_SIZE || read_size != write_size) break;
    }

    chacha_wipe(&ctx);

error:
    if (infile != NULL)
    {
        fclose(infile);
    }

    if (outfile != NULL)
    {
        fclose(outfile);
    }

    if (buffer != NULL)
    {
        free(buffer);
    }
}

void chacha_decode_filepath(const char* inpath, const char* outpath)
{
    chacha_encode_filepath(inpath, outpath);
}
