#!/usr/bin/env python3
import os
import sys
import argparse
import itertools
import time


def main():
    """
    Encode and decode files using the RLE codec.

    use -h or --help for more info
    """

    # Handle argument parsing
    parser = argparse.ArgumentParser()
    parser.add_argument('file', metavar='INFILE', nargs='+', default=[], help='file to compress or decompress')
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
            if not inpath.endswith('.rle'):
                if not outpath.endswith('.rle'):
                    outpath = outpath + '.rle'

                rle_encode(inpath, outpath)
            else:
                print('error attempting to encode already compressed file')
                sys.exit(1)
        else:
            if inpath.endswith('.rle'):
                if outpath == inpath:
                    outpath = outpath.rsplit('.', 1)[0]

                rle_decode(inpath, outpath)
            else:
                print('error attempting to decode non rle file')
                sys.exit(1)

        if not args.keep:
            os.remove(inpath)

        timediff = time.time() - timediff
        new_size = os.path.getsize(outpath)

        if args.verbosity > 0:
            print('file: %s -> %s' % (inpath, outpath))

        if args.verbosity > 1:
            print('size: %u -> %u (%.0f:1) [%.1f%% Compressed]' % (old_size, new_size, max(old_size, new_size) / min(old_size, new_size), (1 - min(old_size, new_size) / max(old_size, new_size)) * 100))
            print('time: %.2f seconds (%.1f %sB/s)' % (timediff, *hr_base2(max(old_size, new_size) / timediff)))

        sys.exit()


def rle_encode(inpath, outpath):
    """
    Encode the input file using the RLE codec.

    Params:
        inpath  - path to input file to read and encode
        outpath - path to output file to write encoded data
    """

    # Open files for processing
    infile = open(inpath, 'rb')
    outfile = open(outpath, 'wb')
    size = os.path.getsize(inpath)

    # Initialize symbol trackers and counter
    (start, current, count) = (b'', b'', 1)

    # Start compression using RLE codec
    if size:
        start = infile.read(1)
        size -= 1

    # Scan input for runs of repeated symbols
    while size:
        current = infile.read(1)
        size -= 1

        if current != start or count == 255:
            outfile.write(bytes([count & 0xFF]))
            outfile.write(start)
            start = current
            count = 1
        else:
            count += 1

    # Output final run
    outfile.write(bytes([count & 0xFF]))
    outfile.write(start)

    infile.close()
    outfile.close()


def rle_decode(inpath, outpath):
    """
    Decode the input file using the RLE codec.

    Params:
        infile  - path to input file to read and decode
        outfile - path to output file to write decoded data
    """

    # Open files for processing
    infile = open(inpath, 'rb')
    outfile = open(outpath, 'wb')
    size = os.path.getsize(inpath)

    # Start decompression using RLE codec
    while size:
        count = infile.read(1)[0]
        symbol = infile.read(1)
        size -= 2

        outfile.write(symbol * count)

    infile.close()
    outfile.close()


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
