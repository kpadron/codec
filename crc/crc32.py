#!/usr/bin/env python3
import os
import sys

sys.path.append('../shared')
import utility as util

# Hash 1 MiB blocks from the file at a time
BLOCK_SIZE = (1 << 20)

def _crc_8(data: int, polynomial: int) -> int:
    """Manually calculates the 32-bit CRC value for the provided 8-bit input
    using the given reversed form CRC polynomial.
    """
    polynomial &= 0xFFFFFFFF
    crc = data & 0xFF

    for _ in range(8):
        if not (crc & 1): crc = (crc >> 1) & 0x7FFFFFFF
        else: crc = ((crc >> 1) & 0x7FFFFFFF) ^ polynomial

    return crc


def _crc_32(data: bytes, crc: int, table: list) -> int:
    """Calculates the 32-bit CRC value for the provided input bytes.

    This computes the CRC values using the provided reversed form CRC table.
    """
    crc = (crc ^ 0xFFFFFFFF) & 0xFFFFFFFF

    for b in data:
        index = (crc ^ b) & 0xFF
        crc = (crc >> 8) & 0xFFFFFF
        crc ^= table[index]

    return (crc ^ 0xFFFFFFFF) & 0xFFFFFFFF


def generate_crc32_table(polynomial: int) -> list:
    """Generate a lookup table containing the 32-bit CRC values of all 8-bit inputs
    using the given reversed form CRC polynomial.
    """
    return [_crc_8(b, polynomial) for b in range(256)]


def crc32(data: bytes, crc: int = 0) -> int:
    """Calculate the CRC-32 value for the provided input bytes.
    """
    return _crc_32(data, crc, crc32_table)


def crc32c(data: bytes, crc: int = 0) -> int:
    """Calculate the CRC-32C (Castagnoli) value for the provided input bytes.
    """
    return _crc_32(data, crc, crc32c_table)


def crc_file(inpath):
    """Calculate the CRC value for the provided input file.
    """
    crc = 0

    with open(inpath, 'rb') as infile:
        buffer = bytearray(BLOCK_SIZE)

        while True:
            read_size = infile.readinto(buffer)

            with memoryview(buffer)[:read_size] as view:
                crc = crc32(view, crc)

            if read_size != len(buffer):
                break

    return crc


# CRC-32 Table
# Uses CRC polynomial 0x04C11DB7 (or 0xEDB88320 in reversed form)
# This is used in Ethernet, SATA, and other protocols, formats, and systems.
crc32_table = generate_crc32_table(0xEDB88320)


# CRC-32C (Castagnoli) Table
# Uses CRC polynomial 0x1EDC6F41 (or 0x82F63B78 in reversed form)
# This is used in SCTP, ext4, Btrfs, and other protocols, formats, and systems.
crc32c_table = generate_crc32_table(0x82F63B78)


if __name__ == '__main__':
    import hash

    sys.exit(hash.hash_main('CRC-32', crc_file))
