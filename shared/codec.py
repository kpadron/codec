#!/usr/bin/env python3
import os
import time
import argparse
import itertools

import utility as util

VERBOSITY = 0

def codec_main(codec_name, codec_extension, encoder, decoder):
    """Common handling and argument parsing for a codec interface.

    Params:
        codec_name - string of the codec being used
        codec_extension - the filename extension to use for this codec
        encoder - function that will encode a input file to a output file
        decoder - function that will decode a input file to a output file
    """
    global VERBOSITY

    # Handle argument parsing
    parser = argparse.ArgumentParser()
    parser.add_argument('infile', metavar='INFILE', nargs='+', default=[], help='file(s) to encode or decode')
    parser.add_argument('-d', '--decode', action='store_true', default=False, help='decode file(s) in list')
    parser.add_argument('-c', '--clobber', action='store_true', default=False, help='remove original file(s) when finished')
    parser.add_argument('-o', '--outfile', metavar='OUTFILE', nargs='+', default=[], help='rename output file to %(metavar)s')
    parser.add_argument('-v', '--verbosity', action='count', default=0, help='increase output verbosity')
    args = parser.parse_args()

    VERBOSITY = args.verbosity

    # Loop through file list
    for (inpath, outpath) in itertools.zip_longest(args.infile, args.outfile):
        if inpath is None or not os.path.isfile(inpath):
            continue

        if outpath is None:
            outpath = inpath

        # Save start time and starting file size
        old_size = os.path.getsize(inpath)
        timediff = time.time()

        # Encode file using codec encoder
        if not args.decode:
            if not inpath.endswith(codec_extension):
                if not outpath.endswith(codec_extension):
                    outpath += codec_extension

                encoder(inpath, outpath)

            else:
                print(f'{codec_name} error: attempting to encode already encoded file')
                return 1

        # Decode file using codec decoder
        else:
            if inpath.endswith(codec_extension):
                if outpath == inpath:
                    outpath = outpath.rsplit('.', 1)[0]

                decoder(inpath, outpath)

            else:
                print(f'{codec_name} error: attempting to decode non \'{codec_extension}\' file')
                return 2

        # Remove old file if necessary
        if args.clobber:
            os.remove(inpath)

        # Calculate elapsed time and new file size
        timediff = time.time() - timediff
        new_size = os.path.getsize(outpath)

        if VERBOSITY > 0:
            out_str = f'{inpath} {old_size}B -> <{codec_name}> -> {outpath} {new_size}B'

            if VERBOSITY > 1:
                ratio = max(old_size, new_size) / min(old_size, new_size)

                out_str += ' (%.0f:1) [%.1f%% delta]' % (ratio, (1 - (1.0 / ratio)) * 100)
                out_str += ' [%.2f s (%sB/s)]' % (timediff, util.size_fmt(max(old_size, new_size) / timediff))

            print(out_str)

    return 0
