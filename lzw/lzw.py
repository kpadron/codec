#!/usr/bin/env python3
import os
import argparse
import time
import itertools


def main():
    """
    Compress files with lzw codec.

    use -h or --help for more info
    """

    # Handle argument parsing
    parser = argparse.ArgumentParser()
    parser.add_argument('file', metavar='infile', nargs='+', default=[], help='file to compress or decompress')
    parser.add_argument('-d', '--decompress', action='store_true', default=False, help='decompress files in list')
    parser.add_argument('-k', '--keep', action='store_true', default=False, help='keep original files (do not overwrite)')
    parser.add_argument('-o', '--output-file', metavar='OUTFILE', nargs='+', default=[], help='rename output file to %(metavar)s')
    parser.add_argument('-v', '--verbosity', action='count', default=0, help='increase output verbosity')
    args = parser.parse_args()

    # Loop through file list
    for inpath, outpath in itertools.zip_longest(args.file, args.output_file):
        if inpath is None:
            continue

        if outpath is None:
            outpath = inpath

        old_size = os.path.getsize(inpath)
        timediff = time.time()

        # Compress or decompress each file
        if not args.decompress:
            if not inpath.endswith('.lzw'):
                if not outpath.endswith('.lzw'):
                    outpath = outpath + '.lzw'

                lzw_encode(inpath, outpath)
            else:
                print('error attempting to encode already compressed file')
                sys.exit(1)
        else:
            if inpath.endswith('.lzw'):
                if outpath == inpath:
                    outpath = outpath.rsplit('.', 1)[0]

                lzw_decode(inpath, outpath)
            else:
                print('error attempting to decode non lzw file')
                sys.exit(1)

        if not args.keep:
            os.remove(inpath)

        timediff = time.time() - timediff
        new_size = os.path.getsize(outpath)

        if args.verbosity > 0:
            print('file: %s -> %s' % (inpath, outpath))

        if args.verbosity > 1:
            print('size: %u -> %u (%.0f:1)' % (old_size, new_size, max(old_size, new_size) / min(old_size, new_size)))
            print('time: %.2f seconds (%.1f %sB/s)' % (timediff, *hr_bytes(max(old_size, new_size) / timediff)))


def lzw_encode(inpath, outpath):
    """
    Perform LZW codec on file.

    Params:
        inpath  - path to file to read
        outpath - path to file to encode and write
    """

    # Open files for processing
    infile = open(inpath, 'rb')
    outfile = open(outpath, 'wb')
    size = os.path.getsize(inpath)

    # Initialize dictionary with all roots
    dictionary = {bytes([i]): i for i in range(256)}
    max_code = 256
    code_bits = 8

    # Initalize buffer for variable width codes
    buffer = 0
    buffer_bits = 0

    C, P = b'', b''

    # Start compression using LZW codec
    while size:
        # Find longest pattern in the dictionary that matches input
        while size:
            C = infile.read(1)
            size -= 1

            if P + C not in dictionary:
                break

            P = P + C

        # Adjust code width if necessary
        if max_code >= (1 << code_bits):
            code_bits += 1

        # Add P + C to the dictionary
        dictionary[P + C] = max_code
        max_code += 1

        # Emit code for P
        code = dictionary[P]
        buffer |= (code & ((1 << code_bits) - 1)) << buffer_bits
        buffer_bits += code_bits

        # Reset P
        P = C

        # Write buffer blocks to file
        while buffer_bits >= 8:
            outfile.write(bytes([buffer & 0xff]))
            buffer >>= 8
            buffer_bits -= 8

    # No more data output final code for P
    while buffer_bits > 0:
        outfile.write(bytes([buffer & 0xff]))
        buffer >>= 8
        buffer_bits -= 8

    infile.close()
    outfile.close()


def lzw_decode(inpath, outpath):
    """
    Perform LZW codec on file.

    Params:
        infile  - path to file to decode
        outfile - path to file to write
    """

    # Open files for processing
    infile = open(inpath, 'rb')
    outfile = open(outpath, 'wb')
    size = os.path.getsize(inpath)

    # Initialize dictionary with all roots
    dictionary = {i: bytes([i]) for i in range(256)}
    max_code = 256
    code_bits = 8

    # Initialize buffer for variable width codes
    buffer = 0
    buffer_bits = 0

    C, P, X, Y = 0, 0, b'', b''
    once = True

    # Start decompression using LZW codec
    while size:
        # Adjust code width if necessary
        # Note: decoder is always one code behind encoder
        if max_code >= (1 << code_bits) - 1:
            code_bits += 1

        # Read codes from file
        while buffer_bits < code_bits and size:
            b = infile.read(1)
            buffer |= (b[0] & 0xff) << buffer_bits
            buffer_bits += 8
            size -= 1

        # Let P be previous code
        P = C

        # Let C be a new code read from buffer
        C = buffer & ((1 << code_bits) - 1)
        buffer >>= code_bits
        buffer_bits -= code_bits

        # Only output during first iteration
        if once:
            once = False
            outfile.write(dictionary[C])
            continue

        # Let X be the pattern for P
        X = dictionary[P]

        if C in dictionary:
            # Emit pattern for C
            outfile.write(dictionary[C])

            # Let Y be first byte of pattern for C
            Y = dictionary[C][:1]

            # Add X + Y to dictionary
            dictionary[max_code] = X + Y
            max_code += 1
        else:
            # Let Z be first byte of pattern for P
            Z = dictionary[P][:1]

            # Emit pattern X + Z
            outfile.write(X + Z)

            # Add X + Z to dictionary
            dictionary[max_code] = X + Z
            max_code += 1

    infile.close()
    outfile.close()


def hr_bytes(bytes):
    """
    Convert bytes to human readable format.

    Params:
        bytes - number to convert

    Returns:
        scaled - number in new base
        symbol - base symbol
    """

    symbols = ['', 'Ki', 'Mi', 'Gi', 'Ti', 'Pi', 'Ei', 'Zi', 'Yi']
    scale = 1024

    for symbol in symbols:
        if bytes / scale < 1.0:
            break

        scale = scale << 10

    return (bytes / (scale >> 10), symbol)


if __name__ == '__main__':
    main()
