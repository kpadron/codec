#!/usr/bin/env python3
# Read 1 MiB from the a file at a time
BLOCK_SIZE = (1 << 20)

def filepath_bytes(filepath, offset = None, block_size = BLOCK_SIZE):
    """Generator function for reading bytes from a filepath in a for loop.

    Params:
        filepath   - file path to read from
        offset     - number of bytes to start from the beginning of the file
        block_size - the maximum number of bytes to read from the file at a time

    Returns:
        The iterator for reading bytes from a file.
    """
    with open(filepath, 'rb') as file:
        yield from file_bytes(file, offset, block_size)


def file_bytes(file, offset = None, block_size = BLOCK_SIZE):
    """Generator function for reading bytes from a file object in a for loop.

    Params:
        file       - file object to read from
        offset     - number of bytes to start from the beginning of the file
        block_size - the maximum number of bytes to read from the file at a time

    Returns:
        The iterator for reading bytes from a file.
    """
    if offset is not None:
        file.seek(offset)

    buffer = bytearray(block_size)

    while True:
        read_size = file.readinto(buffer)

        with memoryview(buffer)[:read_size] as view:
            yield from view

        if read_size != block_size:
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


def rev_bits(x: int, width: int = 0) -> int:
    """Return the given integer but with the bits reversed.

    Params:
        x     - integer to reverse
        width - width of the integer (in bits)

    Returns:
        The reversed integer.
    """
    if not width:
        width = x.bit_length()

    rev = 0

    for i in range(width):
        bit = x & (1 << i)
        if bit: rev |= (1 << (width - i - 1))

    return rev
