#pragma once
#include "util.h"

// ChaCha Cipher Tunables

// Key size (in bytes) to use for ChaCha cipher.
// MUST be 16 or 32.
#define CHACHA_KEY_SIZE 32

// Number of ChaCha round operations to perform when
// generating keystream blocks. More rounds offer
// higher security at the cost of lower performance.
// MUST be a integer value greater than 0.
// Common values are 8, 12, or 20.
#define CHACHA_ROUNDS 20

// Number of bytes to reserve for the ChaCha cipher
// internal stream counter.
// MUST be 4, 8, or 16.
#define CHACHA_COUNTER_SIZE 16

// ChaCha Cipher Constants [DO NOT CHANGE]

// Nonce size (in bytes) to use for ChaCha cipher.
// MUST be 16.
#define CHACHA_NONCE_SIZE 16

// Number of 32-bit (4-byte) words present in the
// ChaCha cipher internal state buffer.
// MUST be 16.
#define CHACHA_STATE_WORDS 16

// ChaCha cipher context structure
typedef struct chacha_ctx
{
    u32 state[CHACHA_STATE_WORDS];
    u32 stream[CHACHA_STATE_WORDS];
    size_t index;
} chacha_ctx;

// Set initial state for ChaCha cipher context
void chacha_init(
    chacha_ctx* ctx,
    const u8 key[CHACHA_KEY_SIZE]);

// Securely teardown a ChaCHa cipher context
void chacha_wipe(chacha_ctx* ctx);

// Seek the ChaCha keystream using a 128-bit (16-byte) nonce
void chacha_seek(
    chacha_ctx* ctx,
    const u8 nonce[CHACHA_NONCE_SIZE]);

// Seek the ChaCha keystream using a 64-bit (8-byte) nonce and block counter
void chacha_seek_block(chacha_ctx* ctx, u64 nonce, u64 block);

// Seek the ChaCha keystream using a 64-bit (8-byte) nonce and byte counter
void chacha_seek_offset(chacha_ctx* ctx, u64 nonce, u64 offset);

// Crypt data using a ChaCha cipher context
void chacha_update(
    chacha_ctx* ctx,
    const void* input,
    void* output,
    size_t size);

// Crypt data using a key and nonce
void chacha_crypt(
    const u8 key[CHACHA_KEY_SIZE],
    const u8 nonce[CHACHA_NONCE_SIZE],
    const void* input,
    void* output,
    size_t size);

// Crypt data using a key, nonce, and block counter
void chacha_crypt_block(
    const u8 key[CHACHA_KEY_SIZE],
    u64 nonce,
    u64 block,
    const void* input,
    void* output,
    size_t size);

// Crypt data using a key, nonce, and byte counter
void chacha_crypt_offset(
    const u8 key[CHACHA_KEY_SIZE],
    u64 nonce,
    u64 offset,
    const void* input,
    void* output,
    size_t size);
