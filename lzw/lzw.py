#!/usr/bin/env python3
import sys
import struct

sys.path.append('../shared')
import utility as util


def lzw_encode(inpath, outpath):
    """Encode the input file using the LZW codec.

    Params:
        inpath  - path to input file to read and encode
        outpath - path to output file to write encoded data
    """
    # Initialize dictionary with all roots
    dictionary = {bytes([i]): i for i in range(256)}
    max_code = 256
    code_bits = 8

    # Initalize buffer for variable width codes
    buffer = 0
    buffer_bits = 0

    # Start compression using LZW codec
    with open(outpath, 'wb') as outfile:
        P = b''

        # Encode all input symbols
        for b in util.filepath_bytes(inpath):
            # Find longest pattern in the dictionary that matches input
            C = bytes([b])
            PC = P + C # PC = P + C
            if PC in dictionary:
                P = PC # P = P + C
                continue

            # Emit code for P
            code = dictionary[P]
            buffer |= ((code & ((1 << code_bits) - 1)) << buffer_bits)
            buffer_bits += code_bits

            # Add P + C to the dictionary
            dictionary[PC] = max_code
            max_code += 1

            # Adjust code width if necessary
            if max_code >= (1 << code_bits):
                code_bits += 1

            # Reset P
            P = C

            # Write encoded bits to the output file
            while buffer_bits >= 8:
                outfile.write(bytes([buffer & 0xFF]))
                buffer >>= 8
                buffer_bits -= 8

        # No more input data; output final code for P
        code = dictionary.get(P)
        if code is not None:
            buffer |= ((code & ((1 << code_bits) - 1)) << buffer_bits)
            buffer_bits += code_bits

            while buffer_bits > 0:
                outfile.write(bytes([buffer & 0xFF]))
                buffer >>= 8
                buffer_bits -= 8


def lzw_decode(inpath, outpath):
    """Decode the input file using the LZW codec.

    Params:
        infile  - path to input file to read and decode
        outfile - path to output file to write decoded data
    """
    # Initialize dictionary with all roots
    dictionary = {i: bytes([i]) for i in range(256)}
    max_code = 256
    code_bits = 9

    # Initialize buffer for variable width codes
    buffer = 0
    buffer_bits = 0

    # Start decompression using LZW codec
    with open(outpath, 'wb') as outfile:
        infile_bytes = util.filepath_bytes(inpath)

        # Output pattern for first code
        try:
            C = next(infile_bytes)
            outfile.write(dictionary[C])
        except StopIteration:
            return

        # Decode all input codes
        for b in infile_bytes:
            # Fill buffer with encoded bits from the input file
            buffer |= ((b & 0xFF) << buffer_bits)
            buffer_bits += 8

            # Ensure buffer is large enough to hold a code
            if buffer_bits < code_bits:
                continue

            # Let P be the previous code
            P = C

            # Let C be a new code read from the buffer
            C = (buffer & ((1 << code_bits) - 1))
            buffer >>= code_bits
            buffer_bits -= code_bits

            # Let W be the pattern for C
            W = dictionary.get(C)

            # Let X be the pattern for P
            X = dictionary.get(P)

            if W is not None:
                # Emit pattern for C
                outfile.write(W)

                # Let Y be first byte of pattern for C
                Y = W[:1]

                # Create X + Y
                entry = X + Y
            else:
                # Let Z be first byte of pattern for P
                Z = X[:1]

                # Create X + Z
                entry = X + Z

                # Emit pattern X + Z
                outfile.write(entry)

            # Add entry to dictionary
            dictionary[max_code] = entry
            max_code += 1

            # Adjust code width if necessary
            # Note: decoder is always one code behind encoder
            if max_code >= (1 << code_bits) - 1:
                code_bits += 1


if __name__ == '__main__':
    import codec

    sys.exit(codec.codec_main('LZW', '.Z', lzw_encode, lzw_decode))
