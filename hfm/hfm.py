#!/usr/bin/env python3
import sys
import struct

sys.path.append('../shared')
import utility as util

DEBUG_PRINT_BUFFER = False
DEBUG_PRINT_TREE = False
DEBUG_PRINT_TABLE = False

def hfm_encode(inpath, outpath):
    """Encode the input file using the Huffman codec.

    Params:
        inpath  - path to input file to read and encode
        outpath - path to output file to write encoded data
    """
    # Calculate the frequency of all symbols in the input file
    freq_table = {}
    for symbol in util.filepath_bytes(inpath):
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
        for symbol in util.filepath_bytes(inpath):
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
    """Decode the input file using the Huffman codec.

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
        buffer = buffer_bits = size = 0
        rover = tree

        # Decode symbols using variable width bit codes
        for code in util.filepath_bytes(inpath, offset=data_offset):
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
    info_format = '!H'
    entry_format = '!BB'
    data_formats = ['!B', '!H', '!I', '!Q']

    info_struct = struct.Struct(info_format)
    entry_struct = struct.Struct(entry_format)

    @classmethod
    def write(cls, outfile, freq_table):
        """Write the Huffman frequency table and header information to
        the encoded output file.
        """
        # Write number of frequency table entries to the output file
        outfile.write(cls.info_struct.pack(len(freq_table)))

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
        """Read the Huffman frequency table and header information from
        the encoded input file.
        """
        # Initialize symbol frequency table
        freq_table = {}
        decoded_size = 0
        header_size = 0

        with open(inpath, 'rb') as infile:
            # Unpack file header info from the encoded input file
            freq_entries = cls.info_struct.unpack(infile.read(cls.info_struct.size))[0]

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
    """A tree that doubles as a linked list.
    Used to generate Huffman codes based on symbol frequency.
    """

    class TreeNode:
        """Simple binary tree node object."""
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
        """Generates a binary Huffman tree based on symbol frequency.
        Returns the tree root and a list of the leaf nodes.
        """
        # Initialize sorted queue and leaf list
        queue = []
        leaves = []
        tree = None

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

        if len(queue):
            tree = queue[0]

        # Print debugging information
        if DEBUG_PRINT_TREE:
            cls.tree_print(tree)

        return (tree, leaves)

    @staticmethod
    def insort(sorted_list, new_item):
        """Simple insertion sort algorithm."""
        for (index, item) in enumerate(sorted_list):
            if new_item < item:
                sorted_list.insert(index, new_item)
                return

        sorted_list.append(new_item)

    @staticmethod
    def leaf_code(leaf):
        """Determine the binary encoding of the symbol at this leaf
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
        """Prints to stdout a tree for debugging."""
        if node is None: return
        cls.tree_print(node.right, level + 1)

        for _ in range(1, level): print('       ', end='')
        if level: print('%d------' % (level - 1), end='')
        if node.up:
            if node is node.up.left: print('\\', end='')
            elif node is node.up.right: print('/', end='')

        print('(%d)' % (node.value), end='')
        if node.left is None or node.right is None:
            char = _filter_symbol(chr(node.key))
            print(' 0x%X %3d \'%s\'' % (node.key, node.key, char), end='')

        print()
        cls.tree_print(node.left, level + 1)

    @classmethod
    def list_print(cls, tree_list):
        """Prints to stdout a list for debugging."""
        for item in tree_list:
            print(id(item))
            cls.tree_print(item)


def hfm_generate_codes(freq_table):
    """Generates a binary Huffman tree based on symbol frequency and
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
        code_table[leaf.key] = (util.rev_bits(code, width), width)

    # Print debugging information
    if DEBUG_PRINT_TABLE:
        for (symbol, (code, width)) in code_table.items():
            char = _filter_symbol(chr(symbol))
            print('[\'%2s\' %3d 0x%2X] (%d) = %d %d %s' % (char, symbol, symbol, freq_table[symbol], code, width, bin(code & ((1 << width) - 1))))

    return code_table


def _filter_symbol(c):
    """Makes control characters more human readable.
    """
    if c in {'\n', '\r', '\t'}:
        return ''.join(['\\', c])

    elif not c.isprintable():
        return '.'

    else:
        return c


if __name__ == '__main__':
    import codec

    sys.exit(codec.codec_main('Huffman', '.hfm', hfm_encode, hfm_decode))
