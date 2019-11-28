#pragma once

#include "util.h"

// CRC-32 Table
// Uses CRC polynomial 0x04C11DB7 (or 0xEDB88320 in reversed form)
// This is used in Ethernet, SATA, and other protocols, formats, and systems.
extern const u32 CRC32_TABLE[256];

// CRC-32C (Castagnoli) Table
// Uses CRC polynomial 0x1EDC6F41 (or 0x82F63B78 in reversed form)
// This is used in SCTP, ext4, Btrfs, and other protocols, formats, and systems.
extern const u32 CRC32C_TABLE[256];

// Calculate the CRC-32 value of the provided data
u32 crc32(const void * const restrict data, size_t size);

// Calculate the CRC-32C value of the provided data
u32 crc32c(const void * const restrict data, size_t size);

// Calculate the CRC-32 value of the file at the provided path
u32 crc32_filepath(const char * const restrict path);

// Calculate the CRC-32C value of the file at the provided path
u32 crc32c_filepath(const char * const restrict path);
