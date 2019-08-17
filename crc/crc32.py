#!/usr/bin/env python3
import os
import sys
import argparse
import time

# Hash 10 MiB blocks from the file
CRC_BLOCK_SIZE = (10 << 20)


def main():
    """
    Generate CRC32 checksum for input files.

    use -h or --help for more info
    """

    # Handle argument parsing
    parser = argparse.ArgumentParser()
    parser.add_argument('file', metavar='INFILE', nargs='+', default=[], help='file to compute the CRC32 for')
    parser.add_argument('-c', '--castagnoli', action='store_true', default=False, help='use the CRC-32C (Castagnoli) polynomial')
    parser.add_argument('-v', '--verbosity', action='count', default=0, help='increase output verbosity')
    args = parser.parse_args()

    # Set the CRC-32 function to use
    crc_func = crc32c if args.castagnoli else crc32

    # Loop through file list
    for inpath in args.file:
        if os.path.isdir(inpath):
            continue

        size = os.path.getsize(inpath)
        timediff = time.time()

        crc = 0

        with open(inpath, 'rb') as f:
            while (size):
                read_size = min(size, CRC_BLOCK_SIZE)
                buffer = f.read(read_size)
                crc = crc_func(buffer, crc)
                size -= read_size

        timediff = time.time() - timediff
        print('%s [%.1f %sB] 0x%X %d' % (inpath, *hr_base2(os.path.getsize(inpath)), crc, crc))

        if args.verbosity > 0:
            print('time: %.2f seconds (%.1f %sB/s)' % (timediff, *hr_base2(os.path.getsize(inpath) / timediff)))

    sys.exit()


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


# CRC-32 Table
# Uses CRC polynomial 0x04C11DB7 (or 0xEDB88320 in reversed form)
# This is used in Ethernet, SATA, and other protocols, formats, and systems.
crc32_table = generate_crc32_table(0xEDB88320)

# CRC-32C (Castagnoli) Table
# Uses CRC polynomial 0x1EDC6F41 (or 0x82F63B78 in reversed form)
# This is used in SCTP, ext4, Btrfs, and other protocols, formats, and systems.
crc32c_table = generate_crc32_table(0x82F63B78)


def hr_base2(x):
    """
    Convert a unit-less number to the equivalent scaled value using standard units (useful for human readable byte sizes).
    Based on powers of 2 (ie Kilo is 2^10 not 10^3).

    Params:
        x - integer number to scale

    Returns:
        scaled - number in new base
        symbol - unit base symbol string
    """

    symbols = ('', 'Ki', 'Mi', 'Gi', 'Ti', 'Pi', 'Ei', 'Zi', 'Yi')
    scale = 1024
    unit = scale

    for symbol in symbols:
        if x / unit < 1.0:
            break

        unit *= scale

    return (x / (unit / scale), symbol)


if __name__ == '__main__':
	main()
