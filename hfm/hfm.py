#!/usr/bin/env python3
import os
import sys
import argparse
import itertools
import time
import struct

# File extension to append when encoded
CODEC_EXT = '.hfm'

# Debugging switches
DEBUG_PRINT_BUFFER = False
DEBUG_PRINT_TREE = False
DEBUG_PRINT_TABLE = False


def main():
    """
    Encode and decode files using the Huffman codec.

    use -h or --help for more info
    """

    # Handle argument parsing
    parser = argparse.ArgumentParser()
    parser.add_argument('file', metavar='INFILE', nargs='+', default=[], help='file to compress or decompress')
    parser.add_argument('-d', '--decompress', action='store_true', default=False, help='decompress files in list')
    parser.add_argument('-c', '--clobber', action='store_true', default=False, help='overwrite original file when finished')
    parser.add_argument('-o', '--output-file', metavar='OUTFILE', nargs='+', default=[], help='rename output file to %(metavar)s')
    parser.add_argument('-v', '--verbosity', action='count', default=0, help='increase output verbosity')
    args = parser.parse_args()

    # Loop through file list
    for inpath, outpath in itertools.zip_longest(args.file, args.output_file):
        if inpath is None or not os.path.isfile(inpath):
            continue

        if outpath is None:
            outpath = inpath

        old_size = os.path.getsize(inpath)
        timediff = time.time()

        if args.verbosity > 2:
            global DEBUG_PRINT_TABLE
            DEBUG_PRINT_TABLE = True

        if args.verbosity > 3:
            global DEBUG_PRINT_TREE
            DEBUG_PRINT_TREE = True

        if args.verbosity > 4:
            global DEBUG_PRINT_BUFFER
            DEBUG_PRINT_BUFFER = True

        # Compress or decompress each file
        if not args.decompress:
            if not inpath.endswith(CODEC_EXT):
                if not outpath.endswith(CODEC_EXT):
                    outpath = outpath + CODEC_EXT

                hfm_encode(inpath, outpath)
            else:
                print('error: attempting to encode already compressed file')
                sys.exit(1)
        else:
            if inpath.endswith(CODEC_EXT):
                if outpath == inpath:
                    outpath = outpath.rsplit('.', 1)[0]

                hfm_decode(inpath, outpath)
            else:
                print('error: attempting to decode non %s file' % (CODEC_EXT))
                sys.exit(2)

        if args.clobber:
            os.remove(inpath)

        timediff = time.time() - timediff
        new_size = os.path.getsize(outpath)

        if args.verbosity > 0:
            print('file: %s -> %s' % (inpath, outpath))

        if args.verbosity > 1:
            print('size: %u -> %u (%.0f:1) [%.1f%% Compressed]' % (old_size, new_size, max(old_size, new_size) / min(old_size, new_size), (1 - min(old_size, new_size) / max(old_size, new_size)) * 100))
            print('time: %.2f seconds (%s/s)' % (timediff, size_fmt(max(old_size, new_size) / timediff)))


        sys.exit()


def hfm_encode(inpath, outpath):
    """
    Encode the input file using the Huffman codec.

    Params:
        inpath  - path to input file to read and encode
        outpath - path to output file to write encoded data
    """

    # Calculate the frequency of all symbols in the input file
    freq_table = {}
    for symbol in filepath_bytes(inpath):
        if symbol not in freq_table:
            freq_table[symbol] = 0

        freq_table[symbol] += 1

    # Generate huffman codes using a binary tree
    code_table = hfm_generate_codes(freq_table)

    # Open output file for encoding
    with open(outpath, 'wb') as outfile:
        # Write Huffman encoding header to the output file
        HfmFileHeader.write(outfile, freq_table)

        # Encode symbols using variable width bit codes
        buffer = buffer_bits = counter = 0
        for symbol in filepath_bytes(inpath):
            # Bit shift corresponding bit code into buffer
            (code, width) = code_table[symbol]
            buffer |= ((code & ((1 << width) - 1)) << buffer_bits)
            buffer_bits += width

            # Write buffer to file when buffer is large enough
            while buffer_bits >= 8:
                # Print debugging information
                if DEBUG_PRINT_BUFFER:
                    print('%d: %s %s' % (counter, bytes([buffer & 0xFF]), bin(buffer & 0xFF)))
                    print()
                    counter += 1

                outfile.write(bytes([buffer & 0xFF]))
                buffer >>= 8
                buffer_bits -= 8
                buffer &= (1 << buffer_bits) - 1

        # Output any leftover bit codes
        if buffer_bits:
            # Print debugging information
            if DEBUG_PRINT_BUFFER:
                print('%d: %s %s' % (counter, bytes([buffer & 0xFF]), bin(buffer & 0xFF)))
                print()

            outfile.write(bytes([buffer & 0xFF]))


def hfm_decode(inpath, outpath):
    """
    Decode the input file using the Huffman codec.

    Params:
        infile  - path to input file to read and decode
        outfile - path to output file to write decoded data
    """

    # Read Huffman encoding header from the output file
    (freq_table, decoded_size, data_offset) = HfmFileHeader.read(inpath)

    # Generate a huffman binary tree used for decoding
    (tree, _) = HfmTree.generate(freq_table)

    # Open file for decoding
    with open(outpath, 'wb') as outfile:
        # Initialize buffer for variable width codes
        buffer = buffer_bits = size = 0
        rover = tree

        # Decode symbols using variable width bit codes
        for code in filepath_bytes(inpath, data_offset):
            # Bit shift corresponding bit code into buffer
            buffer |= (code << buffer_bits)
            buffer_bits += 8

            # Read from buffer and decode into symbols
            while buffer_bits and size < decoded_size:
                # Read a bit from the buffer to determine the direction
                # to go down the tree
                if buffer & 1: rover = rover.right
                else: rover = rover.left

                # Found the leaf symbol
                if rover.left is None and rover.right is None:
                    if DEBUG_PRINT_BUFFER:
                        print('%d: 0x%X %d' % (size, rover.symbol, rover.symbol))

                    outfile.write(bytes([rover.key & 0xFF]))
                    rover = tree
                    size += 1

                # Shift bit out of buffer
                buffer >>= 1
                buffer_bits -= 1
                buffer &= (1 << buffer_bits) - 1

            # Exit if we have decoded all of the symbols
            if size >= decoded_size:
                break


class HfmFileHeader:
    info_format = '!B'
    entry_format = '!BB'
    data_formats = ('!B', '!H', '!I', '!Q')

    info_struct = struct.Struct(info_format)
    entry_struct = struct.Struct(entry_format)

    @classmethod
    def write(cls, outfile, freq_table):
        """
        Write the Huffman frequency table and header information to
        the encoded output file.
        """

        # Write number of frequency table entries to the output file
        outfile.write(cls.info_struct.pack(len(freq_table) - 1))

        # Write frequency table to the output file
        for symbol in range(256):
            freq = freq_table.get(symbol)
            if freq is None:
                continue

            # Determine the minimum number of bytes required to
            # represent the frequency as an unsigned integer
            width = 1
            while freq > (1 << (8 * width)) - 1:
                width <<= 1

            # Write frequency entry to the output file
            entry = cls.entry_struct.pack(symbol, width)
            entry = entry + struct.pack(cls.data_formats[width.bit_length() - 1], freq)
            outfile.write(entry)




    @classmethod
    def read(cls, inpath):
        """
        Read the Huffman frequency table and header information from
        the encoded input file.
        """

        # Initialize symbol frequency table
        freq_table = {}
        decoded_size = 0
        header_size = 0

        with open(inpath, 'rb') as infile:
            # Unpack file header info from the encoded input file
            freq_entries = cls.info_struct.unpack(infile.read(cls.info_struct.size))[0] + 1

            # Read frequency table entries from the file header
            for _ in range(freq_entries):
                # Unpack symbol and frequency data width
                (symbol, width) = cls.entry_struct.unpack(infile.read(cls.entry_struct.size))

                # Unpack frequency entry for this symbol
                data_struct = struct.Struct(cls.data_formats[width.bit_length() - 1])
                freq = data_struct.unpack(infile.read(data_struct.size))[0]

                # Add entry to frequency table
                freq_table[symbol] = freq
                decoded_size += freq

            header_size = infile.tell()

        return (freq_table, decoded_size, header_size)


class HfmTree:
    """
    A tree that doubles as a linked list.
    Used to generate Huffman codes based on symbol frequency.
    """

    class TreeNode:
        """Simple binary tree node object"""

        def __init__(self, key, value, left = None, right = None, up = None):
            self.key = key
            self.value = value
            self.left = left
            self.right = right
            self.up = up

        def __lt__(self, other):
            return self.value < other.value

        def __str__(self):
            return '[%s, %s]' % (str(self.key), str(self.value))

        def __repr__(self):
            return str(self)

    @classmethod
    def generate(cls, freq_table):
        """
        Generates a binary Huffman tree based on symbol frequency.
        Returns the tree root and a list of the leaf nodes.
        """

        # Initialize sorted queue and leaf list
        queue = []
        leaves = []

        # Create a leaf node for each symbol and add it to a sorted list (ascending stable)
        for symbol in range(256):
            freq = freq_table.get(symbol)
            if freq is None:
                continue

            leaf = cls.TreeNode(symbol, freq)
            cls.insort(queue, leaf)
            leaves.append(leaf)

        # Transform sorted list of leaf nodes into a binary tree
        while len(queue) > 1:
            # Extract the two smallest sub-trees in the list
            left = queue[0]
            right = queue[1]
            queue = queue[2:]

            # Combine the two smallest sub-trees into a single tree
            parent = cls.TreeNode(0, left.value + right.value, left, right)
            left.up = right.up = parent

            # Insert new tree into sorted list (ties go right)
            cls.insort(queue, parent)

        tree = queue[0]

        # Print debugging information
        if DEBUG_PRINT_TREE:
            cls.tree_print(tree)

        return (tree, leaves)

    @staticmethod
    def insort(sorted_list, item):
        for i in range(len(sorted_list)):
            if item < sorted_list[i]:
                sorted_list.insert(i, item)
                return

        sorted_list.append(item)


    @staticmethod
    def leaf_code(leaf):
        """
        Determine the binary encoding of the symbol at this leaf
        in a binary Huffman tree.
        """

        code = width = 0
        while leaf is not None and leaf.up is not None:
            if leaf is leaf.up.right:
                code |= (1 << width)

            leaf = leaf.up
            width += 1

        return (code & ((1 << width) - 1), width)


    @classmethod
    def tree_print(cls, node, level = 0):
        """
        Prints to stdout a tree for debugging.
        """

        if node is None: return
        cls.tree_print(node.right, level + 1)

        for _ in range(1, level): print('       ', end='')
        if level: print('%d------' % (level - 1), end='')
        if node.up:
            if node is node.up.left: print('\\', end='')
            elif node is node.up.right: print('/', end='')

        print('(%d)' % (node.value), end='')
        if node.left is None or node.right is None:
            char = chr(node.key)

            if char == '\n':
                char = '\\n'
            elif char == '\r':
                char = '\\r'
            elif char == '\t':
                char = '\\t'
            elif not char.isprintable():
                char = '.'

            print(' 0x%X %3d \'%s\'' % (node.key, node.key, char), end='')

        print()
        cls.tree_print(node.left, level + 1)

    @classmethod
    def list_print(cls, list):
        for item in list:
            print(id(item))
            cls.tree_print(item)


def hfm_generate_codes(freq_table):
    """
    Generates a binary Huffman tree based on symbol frequency and
    returns Huffman code table.
    """

    # Generate the binary Huffman tree
    (_, leaves) = HfmTree.generate(freq_table)

    # Initialize Huffman code table
    code_table = {}

    # Calculate variable-length bit codes using (leaf to root) tree traversal
    for leaf in leaves:
        # Huffman codes have the property of no repeated prefixes
        # however suffixes are used in this case as we are scanning
        # reversed codes from the right aka the lsb.
        (code, width) = HfmTree.leaf_code(leaf)

        # Add symbol code to the Huffman code table
        code_table[leaf.key] = (rev_bits(code, width), width)

    # Print debugging information
    if DEBUG_PRINT_TABLE:
        for (symbol, (code, width)) in code_table.items():
            char = chr(symbol)

            if char == '\n':
                char = '\\n'
            elif char == '\r':
                char = '\\r'
            elif char == '\t':
                char = '\\t'
            elif not char.isprintable():
                char = '.'

            print('[\'%2s\' %3d 0x%2X] (%d) = %d %d %s' % (char, symbol, symbol, freq_table[symbol], code, width, bin(code & ((1 << width) - 1))))

    return code_table


def rev_bits(integer, width):
    """
    Return the given integer but with the bits reversed.
    """

    rev = 0

    for i in range(width):
        bit = integer & (1 << i)
        if bit: rev |= (1 << (width - i - 1))

    return rev & ((1 << width) - 1)

def filepath_bytes(filepath, offset = None, blocksize = (1 << 20)):
    """
    Generator function for reading bytes from a filepath in a for loop.
    """

    with open(filepath, 'rb') as f:
        yield from fileobj_bytes(f, offset, blocksize)

def fileobj_bytes(fileobj, offset = None, blocksize = (1 << 20)):
    """
    Generator function for reading bytes from a file object in a for loop.
    """

    if offset is not None:
        fileobj.seek(offset)

    while True:
        block = fileobj.read(blocksize)

        if not block:
            break

        yield from block

def size_fmt(size, suffix = 'iB', scale = 1024):
    """
    Format a size into a more human readable format.

    Params:
        size - integer number to scale
        suffix - string to add after unit symbol
        scale - the scaling factor between units

    Returns:
        The formatted size string.
    """

    for unit in ('', 'K', 'M', 'G', 'T', 'P', 'E', 'Z'):
        if abs(size) < scale:
            return '%3.1f %s%s' % (size, unit, suffix)

        size /= scale

    return '%.1f Y%s' % (size, suffix)


if __name__ == '__main__':
    main()
