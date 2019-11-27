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
    return ChaCha(key, nonce).crypt(data)

# Return crypted data base on key, nonce, and block counter
def crypt_block(key: bytes, nonce: int, block: int, data: bytes):
    ctx = ChaCha(key)
    ctx.seek_block(nonce, block)
    return ctx.crypt(data)

# Return crypted data base on key, nonce, and byte offset
def crypt_offset(key: bytes, nonce: int, offset: int, data: bytes):
    ctx = ChaCha(key)
    ctx.seek_offset(nonce, offset)
    return ctx.crypt(data)


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
        key = verify_type('key', key, verify_bytes)
        key_size = verify_value('key', len(key), verify_int, CHACHA_KEY_SIZE)

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
        nonce = verify_type('nonce', nonce, verify_bytes)
        nonce_size = verify_value('nonce', len(nonce), verify_int, CHACHA_NONCE_SIZE)

        # Load into state words in little-endian byte order
        for i in range(12, 16):
            offset = (i - 12) * 4
            self.state[i] = load32_le(nonce[offset:offset+4])

        # Reset keystream window index
        self.index = CHACHA_BLOCK_SIZE

    # Return crypted data
    def crypt(self, data: bytes):
        data = verify_type('data', data, verify_bytes)

        if not len(data):
            return b''

        data = memoryview(bytearray(data))

        self.update(data)

        return bytes(data)

    # Seek the ChaCha keystream using a 64-bit (8-byte) nonce and block counter
    def seek_block(self, nonce: int, block: int):
        # Validate nonce input
        nonce = verify_type('nonce', nonce, verify_int)
        nonce = verify_value('nonce', nonce, verify_int, 0, U64_MAX)

        # Validate block input
        block = verify_type('block', block, verify_int)
        block = verify_value('block', block, verify_int, 0, U64_MAX)

        # Load into state words in little-endian byte order
        self.state[12] = block & U32_MAX
        self.state[13] = (block >> 32) & U32_MAX
        self.state[14] = nonce & U32_MAX
        self.state[15] = (nonce >> 32) & U32_MAX

        # Reset keystream window index
        self.index = CHACHA_BLOCK_SIZE

    # Seek the ChaCha keystream using a 64-bit (8-byte) nonce and offset counter
    def seek_offset(self, nonce: int, offset: int):
        # Validate offset input
        offset = verify_type('offset', offset, verify_int)
        offset = verify_value('offset', offset, verify_int, 0, U64_MAX)

        # Determine keystream block and index
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
        # Validate input data
        data = verify_type('data', data, verify_bytes_mutable)
        data_size = len(data)

        offset = 0
        stream_size = CHACHA_BLOCK_SIZE - self.index

        # Crypt data using any available keystream bytes
        if data_size > 0 and stream_size > 0:
            # Calculate the maximum size for this operation
            min_size = min(data_size, stream_size)

            # Xor data with keystream bytes
            memxor(data, self.stream[self.index:self.index+min_size], min_size)

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
            memxor(data[offset:offset+data_size], self.stream[:data_size], data_size)

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
    output = memoryview(array.array('I', [0] * CHACHA_STATE_WORDS))
    output[:] = input[:CHACHA_STATE_WORDS]

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

def verify_type(name, var, ver_func, *args, **kwargs):
    try:
        var = ver_func(var, *args, **kwargs)
    except TypeError as exc:
        raise TypeError(f'"{name}" {exc}')
    return var

def verify_value(name, var, ver_func, *args, **kwargs):
    try:
        var = ver_func(var, *args, **kwargs)
    except ValueError as exc:
        raise ValueError(f'"{name}" {exc}')
    return var

def verify_bytes(var):
    try:
        var = memoryview(var)
    except:
        raise TypeError('must be bytes-like object')

    return var

def verify_bytes_mutable(var):
    var = verify_bytes(var)

    if var.readonly:
        raise TypeError('must be mutable')

    return var

def verify_int(var, lower=None, upper=None):
    var = int(var)

    if lower is not None and var < lower:
        raise ValueError(f'must be larger than {lower}: {var}')

    if upper is not None and var > upper:
        raise ValueError(f'must be smaller than {upper}: {var}')

    return var

def add32(x, y):
    return (x + y) & U32_MAX

def lsh32(value, count):
    return (value << count) & U32_MAX

def rotl32(value, count):
    count &= 31
    return lsh32(value, count) | (value >> (32 - count))

def load32_le(data):
    return struct.unpack('<I', data[:4])[0]

def store32_le(data, value):
    data[:4] = struct.pack('<I', value & U32_MAX)

def memxor(dst, src, size):
    for i in range(size):
        dst[i] ^= src[i]
