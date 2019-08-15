#!/usr/bin/env python3
import os
import argparse
import time
import itertools


def main():
    """
    Compress files with rle codec.

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
            print('size: %u -> %u (%.0f:1)' % (old_size, new_size, max(old_size, new_size) / min(old_size, new_size)))
            print('time: %.2f seconds (%.1f %sB/s)' % (timediff, *hr_bytes(max(old_size, new_size) / timediff)))


def rle_encode_markers(inpath, outpath):
    """
    Perform RLE codec on file.

    Params:
        inpath  - path to file to read
        outpath - path to file to encode and write
    """

    # Open files for processing
    infile = open(inpath, 'rb')
    outfile = open(outpath, 'wb')
    size = os.path.getsize(inpath)

    run_count, literal_count, start, next = 1, 1, b'', b''
    run_flag = 0

    # Start compression using RLE codec
    if size:
        start = infile.read(1)
        start_offset = next_offset = 0
        size -= 1

    # Scan input for runs
    while size:
        if run_flag:
            next = infile.read(1)
            next_offset += 1
            size -= 1

            if start == next and run_count < 128:
                run_count += 1
            else:
                run_count |= 0x80
                outfile.write(bytes([run_count & 0xff]))
                outfile.write(start)
                start = next
                start_offset = next_offset
                run_count = 1

                if run_count < 127:
                    run_flag = not run_flag
        else:
            start = next

            next = infile.read(1)
            next_offset += 1
            size -= 1

            if start != next and literal_count < 128:
                literal_count += 1
            else:
                literal_count &= ~0x80
                outfile.write(bytes([literal_count & 0xff]))
                infile.seek(start_offset)
                outfile.write(infile.read(next_offset - start_offset))
                start = next
                start_offset = next_offset
                literal_count = 1

                if literal_count < 127:
                    run_flag = not run_flag

    # Output final run
    outfile.write(bytes([run_count & 0xff]))
    outfile.write(start)

    infile.close()
    outfile.close()


def rle_encode(inpath, outpath):
    """
    Perform RLE codec on file.

    Params:
        inpath  - path to file to read
        outpath - path to file to encode and write
    """

    # Open files for processing
    infile = open(inpath, 'rb')
    outfile = open(outpath, 'wb')
    size = os.path.getsize(inpath)

    count, start, next = 1, b'', b''

    # Start compression using RLE codec
    if size:
        start = infile.read(1)
        size -= 1

    # Scan input for runs
    while size:
        next = infile.read(1)
        size -= 1

        if start == next and count < 256:
            count += 1
        else:
            outfile.write(bytes([count & 0xff]))
            outfile.write(start)

            start = next
            count = 1

    # Output final run
    outfile.write(bytes([count & 0xff]))
    outfile.write(start)

    infile.close()
    outfile.close()

def rle_decode(inpath, outpath):
    """
    Perform RLE codec on file.

    Params:
        infile  - path to file to decode
        outfile - path to file to write
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
