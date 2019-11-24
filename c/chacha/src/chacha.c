#include "chacha.h"

// Block size (in bytes) of a ChaCha keystream block
// MUST be 64.
#define CHACHA_BLOCK_SIZE 64

// Number of 32-bit (4-byte) words to use for the
// incrementing the internal stream counter.
// MUST be 1, 2, or 4.
#define CHACHA_COUNTER_WORDS (CHACHA_COUNTER_SIZE / 4)

// ChaCha cipher constant value used with
// a 256-bit (32-byte) key.
#define CHACHA_SIGMA ((const u8*) "expand 32-byte k")

// ChaCha cipher constant value used with
// a 128-bit (16-byte) key.
#define CHACHA_TAU ((const u8*) "expand 16-byte k")

#if CHACHA_KEY_SIZE >= 32
#define CHACHA_CONSTANT CHACHA_SIGMA
#else
#define CHACHA_CONSTANT CHACHA_TAU
#endif

// Perform the ChaCha quarter round operation
static void chacha_quarter_round(
    u32 block[CHACHA_STATE_WORDS],
    size_t a,
    size_t b,
    size_t c,
    size_t d)
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

// Generate ChaCha cipher keystream
static void chacha_block(
    const u32 input[CHACHA_STATE_WORDS],
    u32 output[CHACHA_STATE_WORDS])
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

    // Encode generated keystream block
    for (size_t i = 0; i < CHACHA_STATE_WORDS; i++)
    {
        store32_le(output + i, output[i] + input[i]);
    }
}

// Increment 128-bit (16-byte) nonce as little-endian counter
static void chacha_increment(u32 counter[CHACHA_COUNTER_WORDS])
{
    for (size_t i = 0; i < CHACHA_COUNTER_WORDS; i++)
    {
        if (++counter[i])
        {
            break;
        }
    }
}

// Set initial state for ChaCha cipher context
void chacha_init(chacha_ctx* ctx, const u8 key[CHACHA_KEY_SIZE])
{
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

// Securely teardown a ChaCha cipher context
void chacha_wipe(chacha_ctx* ctx)
{
    if (ctx != NULL)
    {
        memwipe(ctx, sizeof(*ctx));
    }
}

// Seek the ChaCha keystream using a 128-bit (16-byte) nonce
void chacha_seek(
    chacha_ctx* ctx,
    const u8 nonce[CHACHA_NONCE_SIZE])
{
    ctx->state[12] = load32_le(nonce + 0);
    ctx->state[13] = load32_le(nonce + 4);
    ctx->state[14] = load32_le(nonce + 8);
    ctx->state[15] = load32_le(nonce + 12);

    ctx->index = CHACHA_BLOCK_SIZE;
}

// Seek the ChaCha keystream using a 64-bit (8-byte) nonce and block counter
void chacha_seek_block(chacha_ctx* ctx, u64 nonce, u64 block)
{
    ctx->state[12] = (u32)(block >> 0);
    ctx->state[13] = (u32)(block >> 32);
    ctx->state[14] = (u32)(nonce >> 0);
    ctx->state[15] = (u32)(nonce >> 32);

    ctx->index = CHACHA_BLOCK_SIZE;
}

// Seek the ChaCha keystream using a 64-bit (8-byte) nonce and byte counter
void chacha_seek_offset(chacha_ctx* ctx, u64 nonce, u64 offset)
{
    const u64 block = offset / CHACHA_BLOCK_SIZE;
    const u64 index = offset % CHACHA_BLOCK_SIZE;

    // Seek to relevant keystream block
    chacha_seek_block(ctx, nonce, block);

    // Seek to relevant keystream byte
    if (index > 0)
    {
        chacha_block(ctx->state, ctx->stream);
        chacha_increment(ctx->state + 12);
        ctx->index = (size_t) index;
    }
}

// Crypt data using a ChaCha cipher context
void chacha_update(
    chacha_ctx* ctx,
    const void* input,
    void* output,
    size_t size)
{
    const u8* const stream = (const u8*) ctx->stream;
    size_t offset = 0;

    // Crypt using any available keystream bytes
    const size_t available_stream_bytes = CHACHA_BLOCK_SIZE - ctx->index;
    if (size > 0 && available_stream_bytes > 0)
    {
        // Calculate the correct size for this operation
        const size_t msize = MIN(size, available_stream_bytes);

        // Xor input with keystream bytes
        memxor(
            OFFSET_PTR(output, offset),
            OFFSET_CPTR(input, offset),
            OFFSET_CPTR(stream, ctx->index),
            msize);

        // Update stream index, offset, and size
        ctx->index += msize;
        offset += msize;
        size -= msize;
    }

    // Crypt full blocks
    while (size >= CHACHA_BLOCK_SIZE)
    {
        // Generate next keystream block
        chacha_block(ctx->state, ctx->stream);
        chacha_increment(ctx->state + 12);

        // Xor input with keystream bytes
        memxor(
            OFFSET_PTR(output, offset),
            OFFSET_CPTR(input, offset),
            stream,
            CHACHA_BLOCK_SIZE);

        // Update offset and size
        offset += CHACHA_BLOCK_SIZE;
        size -= CHACHA_BLOCK_SIZE;
    }

    // Crypt last partial block
    if (size > 0)
    {
        // Generate next keystream block
        chacha_block(ctx->state, ctx->stream);
        chacha_increment(ctx->state + 12);

        // Xor input with keystream bytes
        memxor(
            OFFSET_PTR(output, offset),
            OFFSET_CPTR(input, offset),
            stream,
            size);

        // Update stream index
        ctx->index = size;
    }
}

// Crypt data using a key and nonce
void chacha_crypt(
    const u8 key[CHACHA_KEY_SIZE],
    const u8 nonce[CHACHA_NONCE_SIZE],
    const void* input,
    void* output,
    size_t size)
{
    chacha_ctx ctx;

    // Assign key and nonce to context
    chacha_init(&ctx, key);
    chacha_seek(&ctx, nonce);

    // Crypt data with context
    chacha_update(&ctx, input, output, size);

    // Teardown context
    chacha_wipe(&ctx);
}

// Crypt data using a key, nonce, and block counter
void chacha_crypt_block(
    const u8 key[CHACHA_KEY_SIZE],
    u64 nonce,
    u64 block,
    const void* input,
    void* output,
    size_t size)
{
    chacha_ctx ctx;

    // Assign key and seek keystream
    chacha_init(&ctx, key);
    chacha_seek_block(&ctx, nonce, block);

    // Crypt data with context
    chacha_update(&ctx, input, output, size);

    // Teardown context
    chacha_wipe(&ctx);
}

// Crypt data using a key, nonce, and byte counter
void chacha_crypt_offset(
    const u8 key[CHACHA_KEY_SIZE],
    u64 nonce,
    u64 offset,
    const void* input,
    void* output,
    size_t size)
{
    chacha_ctx ctx;

    // Assign key and seek keystream
    chacha_init(&ctx, key);
    chacha_seek_offset(&ctx, nonce, offset);

    // Crypt data with context
    chacha_update(&ctx, input, output, size);

    // Teardown context
    chacha_wipe(&ctx);
}
