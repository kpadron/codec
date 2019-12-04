#!/usr/bin/env python3
CRC32_POLYNOMIAL = 0xEDB88320
CRC32C_POLYNOMIAL = 0x82F63B78

class CrcTable:
    """CRC lookup table that utilizes memoization.
    """

    def __init__(self, polynomial: int = CRC32_POLYNOMIAL):
        """Initialize a CrcTable instance with the specified CRC polynomial.
        """
        self.polynomial = polynomial
        self.table = {}

    def __getitem__(self, index: int) -> int:
        """Return the CRC value for the specified index.
        """
        value = self.table.get(index)

        if value is None:
            value = _crc8(index, self.polynomial)
            self.table[index] = value

        return value


# CRC-32 Table
# Uses CRC polynomial 0x04C11DB7 (or 0xEDB88320 in reversed form)
# This is used in Ethernet, SATA, and other protocols, formats, and systems.
CRC32_TABLE = CrcTable(CRC32_POLYNOMIAL)

# CRC-32C (Castagnoli) Table
# Uses CRC polynomial 0x1EDC6F41 (or 0x82F63B78 in reversed form)
# This is used in SCTP, ext4, Btrfs, and other protocols, formats, and systems.
CRC32C_TABLE = CrcTable(CRC32C_POLYNOMIAL)


def crc32(data: bytes, crc: int = 0) -> int:
    """Calculate the CRC-32 value for the provided input bytes.
    """
    return _crc32(data, crc, CRC32_TABLE)

def crc32c(data: bytes, crc: int = 0) -> int:
    """Calculate the CRC-32C (Castagnoli) value for the provided input bytes.
    """
    return _crc32(data, crc, CRC32C_TABLE)


BUFFER_SIZE = 65536

def crc_file(file_path: str) -> int:
    """Calculate the CRC value for the provided input file.
    """
    with open(file_path, 'rb') as f:
        buffer = memoryview(bytearray(BUFFER_SIZE))
        crc = 0

        while True:
            read_size = f.readinto(buffer)

            crc = crc32(buffer[:read_size], crc)

            if read_size != BUFFER_SIZE:
                break

    return crc


def _crc8(byte, polynomial):
    """Manually calculates the 32-bit CRC value for the provided 8-bit input
    using the given reversed form CRC polynomial.
    """
    polynomial &= 0xFFFFFFFF
    crc = byte & 0xFF

    for bit in range(8):
        xor = crc & 1
        crc = (crc >> 1) & 0x7FFFFFFF

        if xor:
            crc ^= polynomial

    return crc

def _crc32(data, crc, table):
    """Calculates the 32-bit CRC value for the provided input bytes.

    This computes the CRC values using the provided reversed form CRC table.
    """
    crc = ~crc & 0xFFFFFFFF

    for byte in data:
        index = (crc ^ byte) & 0xFF
        crc = (crc >> 8) ^ table[index]

    return ~crc & 0xFFFFFFFF
