#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "chacha.h"

#define FILE_BLOCK_SIZE (1 << 20)

static void memzero(void* data, size_t size)
{
    memset(data, 0, size);
}

static void memzeros(void* data, size_t size)
{
    volatile uint8_t* volatile p = (volatile uint8_t* volatile) data;

    while (size--)
    {
        *p++ = 0;
    }
}

static uint32_t load32_le(const void* data)
{
    const uint8_t* const p = (const uint8_t*) data;

    return ((uint32_t) p[0] <<  0) |
           ((uint32_t) p[1] <<  8) |
           ((uint32_t) p[2] << 16) |
           ((uint32_t) p[3] << 24);
}

static void store32_le(void* data, uint32_t value)
{
    uint8_t* const p = (uint8_t*) data;

    p[0] = (uint8_t) (value >>  0);
    p[1] = (uint8_t) (value >>  8);
    p[2] = (uint8_t) (value >> 16);
    p[3] = (uint8_t) (value >> 24);
}

static uint32_t rotl32(uint32_t value, uint32_t count)
{
    return (value << count) ^ (value >> (32 - count));
}

static const size_t chacha_key_size = 32;
static const size_t chacha_block_size = 64;

static const char chacha_sigma[16] = "expand 32-byte k";
static const char chacha_tau[16] = "expand 16-byte k";

static void chacha_quarter_round(uint32_t block[16], size_t a, size_t b, size_t c, size_t d)
{
    // a += b; d ^= a; d <<<= 16;
    block[a] += block[b];
    block[d] = rotl32(block[a] ^ block[d], 16);

    // c += d; b ^= c; b <<<= 12
    block[c] += block[d];
    block[b] = rotl32(block[c] ^ block[b], 12);

    // a += b; d ^= a; d <<<= 8;
    block[a] += block[b];
    block[d] = rotl32(block[a] ^ block[d], 8);

    // c += d; b ^= c; b <<<= 7;
    block[c] += block[d];
    block[b] = rotl32(block[c] ^ block[b], 7);
}

static void chacha_block(const uint32_t input[16], uint32_t output[16])
{
    memcpy(output, input, chacha_block_size);

    // 10 iterations * 2 rounds per iteration = 20 rounds
    for (size_t i = 0; i < 10; i++)
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

    for (size_t i = 0; i < 16; i++)
    {
        store32_le(&output[i], input[i] + output[i]);
    }
}

static void chacha_increment(uint32_t counter[4])
{
    for (size_t i = 0; i < 4; i++)
    {
        if (++counter[i])
        {
            break;
        }
    }
}

static void chacha_generate_stream(chacha_ctx* ctx)
{
    chacha_block(ctx->state, ctx->stream.u32);
    chacha_increment(&ctx->state[12]);
    ctx->index = 0;
}

void chacha_init(chacha_ctx* ctx, const uint8_t key[32])
{
    const char* const constants = chacha_key_size == 32 ? chacha_sigma : chacha_tau;

    if (ctx == NULL || key == NULL) return;

    ctx->state[0] = load32_le(constants + 0);
    ctx->state[1] = load32_le(constants + 4);
    ctx->state[2] = load32_le(constants + 8);
    ctx->state[3] = load32_le(constants + 12);
    ctx->state[4] = load32_le(key + 0);
    ctx->state[5] = load32_le(key + 4);
    ctx->state[6] = load32_le(key + 8);
    ctx->state[7] = load32_le(key + 12);
    ctx->state[8] = load32_le(key + 16 % chacha_key_size);
    ctx->state[9] = load32_le(key + 20 % chacha_key_size);
    ctx->state[10] = load32_le(key + 24 % chacha_key_size);
    ctx->state[11] = load32_le(key + 28 % chacha_key_size);
    ctx->state[12] = 0;
    ctx->state[13] = 0;
    ctx->state[14] = 0;
    ctx->state[15] = 0;

    ctx->index = chacha_block_size;
}

void chacha_wipe(chacha_ctx* ctx)
{
    if (ctx != NULL)
    {
        memzeros(ctx, sizeof(*ctx));
    }
}

void chacha_start(chacha_ctx* ctx, const uint8_t nonce[16])
{
    if (ctx == NULL || nonce == NULL) return;

    memcpy(&ctx->state[12], nonce, 16);

    ctx->index = chacha_block_size;
}

void chacha_start_block(chacha_ctx* ctx, uint64_t nonce, uint64_t block)
{
    uint8_t tmp[16];

    if (ctx == NULL) return;

    store32_le(tmp + 0, (uint32_t)(block >> 0));
    store32_le(tmp + 4, (uint32_t)(block >> 32));
    store32_le(tmp + 8, (uint32_t)(nonce >> 0));
    store32_le(tmp + 12, (uint32_t)(nonce >> 32));

    chacha_start(ctx, tmp);

    memzeros(tmp, sizeof(tmp));
}

void chacha_start_offset(chacha_ctx* ctx, uint64_t nonce, uint64_t offset)
{
    const uint64_t block = offset / chacha_block_size;
    const size_t index = (size_t)(offset % chacha_block_size);

    if (ctx == NULL) return;

    chacha_start_block(ctx, nonce, block);

    if (index)
    {
        chacha_generate_stream(ctx);
        ctx->index = index;
    }
}

void chacha_update(chacha_ctx* ctx, const void* input, void* output, size_t size)
{
    const uint8_t* in = (const uint8_t*) input;
    uint8_t* out = (uint8_t*) output;

    if (ctx == NULL || in == NULL || out == NULL || size == 0) return;

    while (size--)
    {
        if (ctx->index >= chacha_block_size)
        {
            chacha_generate_stream(ctx);
        }

        *out++ = *in++ ^ ctx->stream.u8[ctx->index++];
    }
}

void chacha_crypt(const uint8_t key[32], const uint8_t nonce[16], const void* input, void* output, size_t size)
{
    chacha_ctx ctx;

    if (key == NULL || nonce == NULL || input == NULL || output == NULL || size == 0) return;

    chacha_init(&ctx, key);
    chacha_start(&ctx, nonce);
    chacha_update(&ctx, input, output, size);
    chacha_wipe(&ctx);
}

void chacha_crypt_block(const uint8_t key[32], uint64_t nonce, uint64_t block, const void* input, void* output, size_t size)
{
    chacha_ctx ctx;

    if (key == NULL || input == NULL || output == NULL || size == 0) return;

    chacha_init(&ctx, key);
    chacha_start_block(&ctx, nonce, block);
    chacha_update(&ctx, input, output, size);
    chacha_wipe(&ctx);
}

void chacha_crypt_offset(const uint8_t key[32], uint64_t nonce, uint64_t offset, const void* input, void* output, size_t size)
{
    chacha_ctx ctx;

    if (key == NULL || input == NULL || output == NULL || size == 0) return;

    chacha_init(&ctx, key);
    chacha_start_offset(&ctx, nonce, offset);
    chacha_update(&ctx, input, output, size);
    chacha_wipe(&ctx);
}

void chacha_encode_buffer(buffer_t buffer, const uint8_t key[32], const uint8_t nonce[16])
{
    chacha_crypt(key, nonce, buffer.data, buffer.data, buffer.size);
}

void chacha_decode_buffer(buffer_t buffer, const uint8_t key[32], const uint8_t nonce[16])
{
    chacha_crypt(key, nonce, buffer.data, buffer.data, buffer.size);
}

void chacha_encode_filepath(const char* inpath, const char* outpath)
{
    FILE* infile = fopen(inpath, "rb");
    FILE* outfile = fopen(outpath, "wb");
    uint8_t* buffer = (uint8_t*) malloc(FILE_BLOCK_SIZE);
    chacha_ctx ctx;

    if (infile == NULL || outfile == NULL || buffer == NULL) goto error;

    chacha_init(&ctx, (const uint8_t[32]){ 0 });
    chacha_start(&ctx, (const uint8_t[16]){ 0 });

    for (;;)
    {
        size_t read_size = fread(buffer, 1, FILE_BLOCK_SIZE, infile);
        size_t write_size = 0;

        chacha_update(&ctx, buffer, buffer, read_size);

        write_size = fwrite(buffer, 1, read_size, outfile);

        if (read_size != FILE_BLOCK_SIZE || read_size != write_size) break;
    }

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
