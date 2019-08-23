#!/usr/bin/env python3
import zlib
import sys

argc = len(sys.argv)
argv = sys.argv

if argc < 2:
    print('Usage: INFILE [OUTFILE]')
    sys.exit()

inpath = argv[1]
outpath = inpath

if argc > 2:
    outpath = argv[2]

with open(inpath, 'rb') as f:
    data = f.read()

size = len(data)
crc = zlib.crc32(data)
adler = zlib.adler32(data)

print('%s (%u)|' % (inpath, size))
print(' crc: %u 0x%X' % (crc, crc))
print(' adler: %u 0x%X' % (adler, adler))

cdata = zlib.compress(data, 9)
csize = len(cdata)
crc = zlib.crc32(cdata)
adler = zlib.adler32(cdata)

print('%s (%u)|' % (outpath, csize))
print(' crc: %u 0x%X' % (crc, crc))
print(' adler: %u 0x%X' % (adler, adler))

with open(outpath, 'wb') as f:
    f.write(cdata)

sys.exit()
