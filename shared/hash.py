#!/usr/bin/env python3
import os
import time
import argparse

import utility as util

VERBOSITY = 0

def hash_main(hash_name, hasher):
    """Common handling and argument parsing for a hash interface (a one-way encoding).

    Params:
        hash_name - string of the hash being used
        hasher - function that will calculate the hash of a file
    """
    global VERBOSITY

    # Handle argument parsing
    parser = argparse.ArgumentParser()
    parser.add_argument('file', metavar='INFILE', nargs='+', default=[], help=f'file(s) to compute the {hash_name} for')
    parser.add_argument('-v', '--verbosity', action='count', default=0, help='increase output verbosity')
    args = parser.parse_args()

    VERBOSITY = args.verbosity

    # Loop through file list
    for inpath in args.file:
        if not os.path.isfile(inpath):
            continue

        # Save start time and file size
        size = os.path.getsize(inpath)
        timediff = time.time()

        # Compute the hash
        h = hasher(inpath)

        # Calculate elapsed time
        timediff = time.time() - timediff

        # Print to stdout
        out_str = f'{inpath} {util.size_fmt(size)}B : <{hash_name}> {hash_str(h)}'

        if VERBOSITY > 0:
            out_str += ' [%.2f s (%sB/s)]' % (timediff, util.size_fmt(size / timediff))

        print(out_str)

    return 0


def hash_str(hash):
    """Convert hash to a printable string.
    """
    if isinstance(hash, int):
        return hex(hash)
    elif isinstance(hash, bytes):
        return hash.hex()
    else:
        return str(hash)
