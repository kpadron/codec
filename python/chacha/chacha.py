import struct
import array

CHACHA_KEY_SIZE = 32
CHACHA_ROUNDS = 20
CHACHA_COUNTER_SIZE = 16

U32_MAX = (2 ** 32) - 1
U64_MAX = (2 ** 64) - 1

CHACHA_BLOCK_SIZE = 64
CHACHA_NONCE_SIZE = 16

CHACHA_STATE_WORDS = CHACHA_BLOCK_SIZE // 4
CHACHA_COUNTER_WORDS = CHACHA_COUNTER_SIZE // 4

CHACHA_SIGMA = b'expand 32-byte k'
CHACHA_TAU = b'expand 16-byte k'
CHACHA_CONSTANT = CHACHA_SIGMA if CHACHA_KEY_SIZE >= 32 else CHACHA_TAU


# Return crypted data based on key and nonce
def crypt(key: bytes, nonce: bytes, data: bytes):
    try:
        mv = memoryview(bytearray(data))

    except TypeError:
        raise TypeError(f'data must be a bytes-like object.')

    ChaCha(key, nonce).update(mv)

    return mv.tobytes()


# ChaCha Symmetric Cipher Context
class ChaCha:
    # Initialize internal state for ChaCha cipher
    def __init__(self, key=None, nonce=None):
        self.state = memoryview(array.array('I', [0] * CHACHA_STATE_WORDS))
        self.stream = memoryview(bytearray(CHACHA_BLOCK_SIZE))
        self.index = CHACHA_BLOCK_SIZE

        if key is not None:
            self.set_key(key)

        if nonce is not None:
            self.set_nonce(nonce)

    # Wipe internal state from memory
    def __del__(self):
        for i in range(CHACHA_STATE_WORDS):
            self.state[i] = 0

        for i in range(CHACHA_BLOCK_SIZE):
            self.stream[i] = 0

        self.index = 0

    # Set the internal ChaCha context using a key
    def set_key(self, key: bytes):
        # Validate input
        try:
            key = memoryview(key)

            if len(key) < CHACHA_KEY_SIZE:
                raise TypeError

        except TypeError:
            raise TypeError(f'key must be a bytes-like object of at least {CHACHA_KEY_SIZE} bytes.')

        # ChaCha constant
        for i in range(4):
            offset = i * 4
            self.state[i] = load32_le(CHACHA_CONSTANT[offset:offset+4])

        # 256-bit (32-byte) or 128-bit (16-byte) key
        for i in range(4, 12):
            offset = ((i - 4) * 4) % CHACHA_KEY_SIZE
            self.state[i] = load32_le(key[offset:offset+4])

        # 128-bit (16-byte) nonce/counter value
        for i in range(12, 16):
            self.state[i] = 0

        # Keystream window index
        self.index = CHACHA_BLOCK_SIZE

    # Seek the ChaCha keystream using a 128-bit (16-byte) nonce value
    def set_nonce(self, nonce: bytes):
        # Validate input
        try:
            nonce = memoryview(nonce)

            if len(nonce) < CHACHA_NONCE_SIZE:
                raise TypeError

        except TypeError:
            raise TypeError(f'nonce must be a bytes-like object of at least {CHACHA_NONCE_SIZE} bytes.')

        # Load into state words in little-endian byte order
        for i in range(12, 16):
            offset = (i - 12) * 4
            self.state[i] = load32_le(nonce[offset:offset+4])

        # Reset keystream window index
        self.index = CHACHA_BLOCK_SIZE

    # Seek the ChaCha keystream using a 64-bit (8-byte) nonce and block counter
    def seek_block(self, nonce: int, block: int):
        if not isinstance(nonce, int) or nonce < 0 or nonce > U64_MAX:
            raise TypeError(f'nonce must be int in range [0, {U_MAX}].')

        if not isinstance(block, int) or block < 0 or block > U64_MAX:
            raise TypeError(f'block must be int in range [0, {U_MAX}].')

        # Load into state words in little-endian byte order
        self.state[12] = block & U32_MAX
        self.state[13] = block >> 32
        self.state[14] = nonce & U32_MAX
        self.state[15] = nonce >> 32

        # Reset keystream window index
        self.index = CHACHA_BLOCK_SIZE

    # Seek the ChaCha keystream using a 64-bit (8-byte) nonce and offset counter
    def seek_offset(self, nonce, offset):
        if not isinstance(nonce, int) or nonce < 0 or nonce > U64_MAX:
            raise TypeError(f'nonce must be int in range [0, {U_MAX}].')

        if not isinstance(offset, int) or offset < 0 or offset > U64_MAX:
            raise TypeError(f'offset must be int in range [0, {U_MAX}].')

        block = offset // CHACHA_BLOCK_SIZE
        index = offset % CHACHA_BLOCK_SIZE

        # Seek to the relevant keystream block
        self.seek_block(nonce, block)

        # Seek to the relevant keystream byte
        if index > 0:
            chacha_block(self.state, self.stream)
            chacha_increment(self.state[12:12+CHACHA_COUNTER_WORDS])
            self.index = index

    # Crypt data using ChaCha cipher
    def update(self, data: bytearray):
        # Validate input
        try:
            data = memoryview(data)

            if data.readonly:
                raise TypeError

        except TypeError:
            raise TypeError(f'data must be a mutable bytes-like object.')

        offset = 0
        data_size = len(data)
        stream_size = CHACHA_BLOCK_SIZE - self.index

        # Crypt data using any available keystream bytes
        if data_size > 0 and stream_size > 0:
            # Calculate the maximum size for this operation
            min_size = min(data_size, stream_size)

            # Xor data with keystream bytes
            memxor(data, self.stream, min_size)

            # Update stream index, offset, and size
            self.index += min_size
            offset += min_size
            data_size -= min_size

        # Crypt full blocks
        while data_size >= CHACHA_BLOCK_SIZE:
            # Generate next keystream block
            chacha_block(self.state, self.stream)
            chacha_increment(self.state[12:12+CHACHA_COUNTER_WORDS])

            # Xor input with keystream bytes
            memxor(data[offset:offset+CHACHA_BLOCK_SIZE], self.stream, CHACHA_BLOCK_SIZE)

            # Update offset and size
            offset += CHACHA_BLOCK_SIZE
            data_size -= CHACHA_BLOCK_SIZE

        # Crypt last partial block
        if data_size > 0:
            # Generate next keystream block
            chacha_block(self.state, self.stream)
            chacha_increment(self.state[12:12+CHACHA_COUNTER_WORDS])

            # Xor input with keystream bytes
            memxor(data[offset:offset+data_size], self.stream, data_size)

            # Update stream index
            self.index = data_size


# Primitive ChaCha operations

# Perform the ChaCha quarter round operation
def chacha_quarter_round(block, a, b, c, d):
    # a += b; d ^= a; d <<<= 16;
    block[a] = add32(block[a], block[b])
    block[d] = rotl32(block[d] ^ block[a], 16)

    # c += d; b ^= c; b <<<= 12;
    block[c] = add32(block[c], block[d])
    block[b] = rotl32(block[b] ^ block[c], 12)

    # a += b; d ^= a; d <<<= 8;
    block[a] = add32(block[a], block[b])
    block[d] = rotl32(block[d] ^ block[a], 8)

    # c += d; b ^= c; b <<<= 7;
    block[c] = add32(block[c], block[d])
    block[b] = rotl32(block[b] ^ block[c], 7)

# Generate ChaCha cipher keystream
def chacha_block(input, stream):
    # Load initial state into output buffer
    output = array.array('I', [0] * CHACHA_STATE_WORDS)
    output.extend(input)

    # Perform ChaCha rounds
    for _ in range(0, CHACHA_ROUNDS, 2):
        # Odd round
        chacha_quarter_round(output, 0, 4, 8, 12)
        chacha_quarter_round(output, 1, 5, 9, 13)
        chacha_quarter_round(output, 2, 6, 10, 14)
        chacha_quarter_round(output, 3, 7, 11, 15)

        # Even round
        chacha_quarter_round(output, 0, 5, 10, 15)
        chacha_quarter_round(output, 1, 6, 11, 12)
        chacha_quarter_round(output, 2, 7, 8, 13)
        chacha_quarter_round(output, 3, 4, 9, 14)

    # Encode generated keystream block
    for i in range(CHACHA_STATE_WORDS):
        offset = i * 4
        store32_le(stream[offset:offset+4], output[i] + input[i])

# Increment 128-bit (16-byte) nonce as little-endian counter
def chacha_increment(counter):
    for i in range(CHACHA_COUNTER_WORDS):
        counter[i] += 1
        if counter:
            break


# Utility functions

def add32(x, y):
    return (x + y) & U32_MAX

def add64(x, y):
    return (x + y) & U64_MAX

def lsh32(value, count):
    return (value << count) & U32_MAX

def lsh64(value, count):
    return (value << count) & U64_MAX

def rotl32(value, count):
    count &= 31
    return lsh32(value, count) | (value >> (32 - count))

def rotl64(value, count):
    count &= 63
    return lsh64(value, count) | (value >> (64 - count))

def load32_le(data):
    return struct.unpack('<I', data[:4])[0]

def store32_le(data, value):
    data[:4] = struct.pack('<I', value & U32_MAX)

def memxor(dst, src, size):
    for i in range(size):
        dst[i] ^= src[i]
