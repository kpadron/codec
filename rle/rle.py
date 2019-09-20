#!/usr/bin/env python3
import sys

sys.path.append('../shared')
import utility as util

def rle_encode(inpath, outpath):
    """
    Encode the input file using the RLE codec.

    Params:
        inpath  - path to input file to read and encode
        outpath - path to output file to write encoded data
    """
    # Open output file for encoding
    with open(outpath, 'wb') as outfile:
        # Initialize sequence tracker and counter
        (start, count) = (b'', 0)

        # Scan input for sequences of repeated symbols
        for symbol in util.filepath_bytes(inpath):
            if symbol == start and count < 255:
                count += 1
            else:
                # Output symbol sequence encoding
                if count > 0:
                    outfile.write(bytes([count]))
                    outfile.write(start)

                # Reset sequence tracker and counter
                (start, count) = (symbol, 1)

        # Output final sequence encoding
        if count > 0:
            outfile.write(bytes([count]))
            outfile.write(start)


def rle_decode(inpath, outpath):
    """
    Decode the input file using the RLE codec.

    Params:
        infile  - path to input file to read and decode
        outfile - path to output file to write decoded data
    """
    # Open output file for decoding
    with open(outpath, 'wb') as outfile:
        # Scan input for (count, symbol) pairs
        for pair in util.filepath_bytes(inpath, 2):
            (count, symbol) = pair
            outfile.write(bytes([symbol] * count))


if __name__ == '__main__':
    import codec

    sys.exit(codec.codec_main('RLE', '.rle', rle_encode, rle_decode))
