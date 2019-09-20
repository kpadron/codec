#!/usr/bin/env python3
# Read 1 MiB from the a file at a time
BLOCK_SIZE = (1 << 20)

def filepath_bytes(filepath, buffer_size = 1, offset = None, block_size = BLOCK_SIZE):
    """Generator function for reading bytes from a filepath in a for loop.

    Params:
        file        - file object to read from
        buffer_size - the maximum size of the returned bytes object per iteration
        offset      - number of bytes to start from the beginning of the file
        block_size  - the maximum number of bytes to read from the file at a time

    Returns:
        The iterator for reading bytes from a file.
    """
    with open(filepath, 'rb') as file:
        yield from file_bytes(file, buffer_size, offset, block_size)


def file_bytes(file, buffer_size = 1, offset = None, block_size = BLOCK_SIZE):
    """Generator function for reading bytes from a file object in a for loop.

    Params:
        file        - file object to read from
        buffer_size - the maximum size of the returned bytes object per iteration
        offset      - number of bytes to start from the beginning of the file
        block_size  - the maximum number of bytes to read from the file at a time

    Returns:
        The iterator for reading bytes from a file.
    """
    if offset is not None:
        file.seek(offset)

    block_size = max(buffer_size, block_size)
    buffer = bytearray(block_size)

    while True:
        read_size = file.readinto(buffer)
        partitions = (read_size + buffer_size - 1) // buffer_size

        for i in range(partitions):
            start = i * buffer_size
            end = min(start + buffer_size, read_size)
            yield bytes(buffer[start:end])

        if read_size < BLOCK_SIZE:
            break


def size_fmt(size: int, scale: int = 1024) -> str:
    """Format a size into a more human readable format.

    Params:
        size  - integer number to scale
        scale - the scaling factor between units

    Returns:
        The formatted size string.
    """
    for unit in ['', 'K', 'M', 'G', 'T', 'P', 'E', 'Z']:
        if abs(size) < scale:
            return '%3.1f %s' % (size, unit)

        size /= scale

    return '%.1f Y' % (size)


def rev_bits(n: int) -> int:
    """Return the given integer but with the bits reversed.
    """
    rev = 0
    while n > 0:
        rev <<= 1
        if (n & 1) == 1:
            rev ^= 1

        n >>= 1

    return rev
