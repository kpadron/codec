// lab4.c
// ECE 4680, Spring 2018
// kpadron@clemson.edu
// Kristian Padron, Evan DeForest

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct node
{
	unsigned char symbol;
	size_t frequency;

	struct node* next;
	struct node* left;
	struct node* right;
} node_t;

typedef struct entry
{
	size_t code;
	size_t length;
} entry_t;

size_t huffman_compress(FILE* in, FILE* out, size_t bytes);
void huffman_decompress(FILE* in, FILE* out, size_t bytes);

size_t huffman_header_write(FILE* out, size_t* frequency_table, size_t total_symbols);
entry_t* huffman_codes(size_t* frequency_table, size_t unique_symbols);
node_t* huffman_generate_tree(size_t* frequency_table, size_t unique_symbols);
size_t huffman_leaf_code(node_t* node, size_t* length);

size_t binary_reverse(size_t binary, size_t bits);
void binary_print(void* ptr, size_t length);

node_t* tree_node(unsigned char symbol, size_t frequency, node_t* next, node_t* left, node_t* right);
void tree_free(node_t* node);
void tree_print(node_t* node, size_t level);

void list_print(node_t* head);
node_t* list_insert_sorted(node_t* head, node_t* tail);


int main(int argc, char** argv)
{
	char compress = 1;
	char* outfile = NULL;

	// parse arguments and determine running mode
	if (argc != 2 && argc != 3)
	{
		printf("Usage: ./lab4 <filename> [d]\n");
		return 1;
	}
	else if (argc == 2) compress = 1;
	else compress = 0;

	// determine output filename
	outfile = (char*) malloc(strlen(argv[1]) + strlen(".dec") + 1);
	outfile = strcpy(outfile, argv[1]);
	if (compress) outfile = strcat(outfile, ".huf");
	else outfile = strcat(outfile, ".dec");

	// open input and output files
	FILE* in = fopen(argv[1], "rb");
	FILE* out = fopen(outfile, "wb");

	// check file errors
	if (in == NULL || out == NULL)
	{
		if (in == NULL) printf("error opening %s for reading\n", argv[1]);
		if (out == NULL) printf("error opening %s for writing\n", outfile);
		return 1;
	}

	// determine file size
	fseek(in, 0L, SEEK_END);
	size_t bytes = ftell(in);
	fseek(in, 0L, SEEK_SET);

	// compress or decompress using huffman codec
	if (compress)
	{
		size_t new_bytes = huffman_compress(in, out, bytes);

		printf("compressed \"%s\" to \"%s\" using huffman codec\n", argv[1], outfile);
		printf("compressed %zu bytes to %zu bytes\n", bytes, new_bytes);
		printf("compression ratio: %.2f\n", (double) bytes / new_bytes);
		printf("size reduction: %.2f%%\n", 100 * (1 - (double) new_bytes / bytes));
	}
	else
	{
		huffman_decompress(in, out, bytes);

		printf("decompressed \"%s\" to \"%s\" using huffman codec\n", argv[1], outfile);
	}

	// cleanup
	free(outfile);
	fclose(in);
	fclose(out);

	return 0;
}


size_t huffman_compress(FILE* in, FILE* out, size_t bytes)
{
	size_t frequency_table[256] = { 0 };
	size_t unique_symbols = 0;
	size_t new_bytes = 0;
	size_t i;

	// calculate frequency of all symbols in the file
	printf("calculating symbol frequencies...\n");
	for (i = 0; i < bytes; i++)
	{
		unsigned char symbol = 0;
		fread(&symbol, sizeof(unsigned char), 1, in);
		if (!frequency_table[symbol]++) unique_symbols++;
	}

	// write header information to output file
	printf("writing header info...\n");
	new_bytes = huffman_header_write(out, frequency_table, bytes);

	// generate huffman codes using a binary tree
	printf("generating huffman tree...\n");
	entry_t* code_table = huffman_codes(frequency_table, unique_symbols);

	// reset input file to the beginning
	fseek(in, 0L, SEEK_SET);
	size_t buffer = 0;
	size_t buffer_bits = 0;

	// encode symbols using varying length bit codes
	printf("encoding file...\n");
	for (i = 0; i < bytes;)
	{
		// fill buffer with symbol codes
		while (buffer_bits < 0.75 * sizeof(size_t) * 8 && i < bytes)
		{
			// read in symbol from input file
			unsigned char symbol = 0;
			fread(&symbol, sizeof(unsigned char), 1, in);

			// bit shift corresponding bit code into buffer
			buffer |= code_table[symbol].code << buffer_bits;
			buffer_bits += code_table[symbol].length;
			i++;
		}

		// write buffer to file when buffer is large enough
		if (buffer_bits >> 3)
		{
			size_t bytes_out = buffer_bits >> 3;
			size_t bits_out = bytes_out << 3;

			fwrite(&buffer, 1, bytes_out, out);
			buffer >>= bits_out;
			buffer_bits -= bits_out;

			new_bytes += bytes_out;
		}

		// printf("progress: %.2f%%\r", (double) i / bytes * 100);
	}
	printf("\n");

	if (buffer_bits)
	{
		fwrite(&buffer, 1, 1, out);
		new_bytes += 1;
	}

	// cleanup
	free(code_table);

	return new_bytes;
}

void huffman_decompress(FILE* in, FILE* out, size_t bytes)
{
	size_t frequency_table[256] = { 0 };
	size_t total_symbols = 0;
	size_t unique_symbols = 0;
	size_t i, j;

	// read in frequency of all symbols in the file
	printf("reading header info...\n");
	for (i = 0; i < 256; i++)
	{
		fread(&frequency_table[i], 4, 1, in);
		if (frequency_table[i]) unique_symbols++;
	}

	// determine total amount of coded symbols in the file
	fread(&total_symbols, 4, 1, in);

	// generate huffman codes using a binary tree
	printf("generating huffman tree...\n");
	node_t* huffman_tree = huffman_generate_tree(frequency_table, unique_symbols);

	size_t buffer = 0;
	size_t buffer_bits = 0;
	node_t* rover = huffman_tree;

	// decode symbols using varying length bit codes
	printf("decoding file...\n");
	for (i = j = 0; i < bytes && j < total_symbols;)
	{
		// fill buffer with codes from file
		while (buffer_bits < sizeof(size_t) * 8 && i < bytes)
		{
			// read in code from input file
			size_t code = 0;
			fread(&code, 1, 1, in);

			// bit shift corresponding bit code into buffer
			buffer |= (code << buffer_bits);
			buffer_bits += 8;
			i++;
		}

		// read from buffer and decode into symbols
		while (buffer_bits && j < total_symbols)
		{
			size_t direction = buffer & 1;

			if (direction) rover = rover->right;
			else rover = rover->left;

			if (!rover->left && !rover->right)
			{
				fwrite(&rover->symbol, sizeof(unsigned char), 1, out);
				rover = huffman_tree;
				j++;
			}

			buffer >>= 1;
			buffer_bits--;
		}
	}
	printf("\n");

	// cleanup
	tree_free(huffman_tree);
}


size_t huffman_header_write(FILE* out, size_t* frequency_table, size_t total_symbols)
{
	size_t i, new_bytes = 0;

	for (i = 0; i < 256; i++)
	{
		fwrite(&frequency_table[i], 4, 1, out);
		new_bytes += 4;
	}

	fwrite(&total_symbols, 4, 1, out);
	new_bytes += 4;

	return new_bytes;
}

// builds binary huffman tree based on symbol frequency and returns code table
entry_t* huffman_codes(size_t* frequency_table, size_t unique_symbols)
{
	entry_t* code_table = NULL;

	if (frequency_table)
	{
		node_t* huffman_tree = NULL;
		node_t** symbol_list = (node_t**) calloc(unique_symbols, sizeof(node_t*));
		size_t i, j;

		// create list of symbols sorted by frequency (ascending stable)
		for (i = j = 0; i < 256; i++)
		{
			if (frequency_table[i])
			{
				node_t* tree = tree_node(i, frequency_table[i], NULL, NULL, NULL);
				huffman_tree = list_insert_sorted(huffman_tree, tree);
				symbol_list[j++] = tree;
			}
		}

		// transform sorted list of symbols into a binary tree
		while (huffman_tree && huffman_tree->next)
		{
			// extract two smallest trees in list
			node_t* left = huffman_tree;
			node_t* right = left->next;
			huffman_tree = huffman_tree->next->next;

			// combine two smallest trees into new tree
			node_t* tree = left->next = right->next = tree_node(0, left->frequency + right->frequency, NULL, left, right);

			// insert new tree into sorted list (ties go right)
			huffman_tree = list_insert_sorted(huffman_tree, tree);
		}

		// print tree progress
		// list_print(huffman_tree);

		// create code table for varying length bit codes
		code_table = (entry_t*) calloc(256, sizeof(entry_t));

		// calculate varying length bit codes using (leaf to root) tree traversal
		for (i = 0; i < unique_symbols; i++)
		{
			unsigned char symbol = symbol_list[i]->symbol;
			size_t code = 0;
			size_t length = 0;

			// huffman codes have the property of no repeated prefixes
			// however suffixes are used in this case as we are scanning
			// reversed codes from the right or lsb
			code = huffman_leaf_code(symbol_list[i], &length);
			code_table[symbol].code = code = binary_reverse(code, length);
			code_table[symbol].length = length;

			// printf("('%c'), %d, %zu, %zu\n", symbol, symbol, code, length);
		}

		// cleanup
		free(symbol_list);
		tree_free(huffman_tree);
	}

	return code_table;
}

node_t* huffman_generate_tree(size_t* frequency_table, size_t unique_symbols)
{
	node_t* huffman_tree = NULL;

	if (frequency_table)
	{
		size_t i, j;
		node_t** symbol_list = (node_t**) calloc(unique_symbols, sizeof(node_t*));

		// create list of symbols sorted by frequency (ascending stable)
		for (i = j = 0; i < 256; i++)
		{
			if (frequency_table[i])
			{
				node_t* tree = tree_node(i, frequency_table[i], NULL, NULL, NULL);
				huffman_tree = list_insert_sorted(huffman_tree, tree);
				symbol_list[j++] = tree;
			}
		}

		// transform sorted list of symbols into a binary tree
		while (huffman_tree && huffman_tree->next)
		{
			// extract two smallest trees in list
			node_t* left = huffman_tree;
			node_t* right = left->next;
			huffman_tree = huffman_tree->next->next;

			// combine two smallest trees into new tree
			node_t* tree = left->next = right->next = tree_node(0, left->frequency + right->frequency, NULL, left, right);

			// insert new tree into sorted list (ties go right)
			huffman_tree = list_insert_sorted(huffman_tree, tree);
		}

		// print tree progress
		// list_print(huffman_tree);

		// cleanup
		free(symbol_list);
	}

	return huffman_tree;
}

size_t huffman_leaf_code(node_t* node, size_t* length)
{
	size_t code = 0;

	for (*length = 0; node && node->next; (*length)++)
	{
		if (node == node->next->right) code |= (1 << *length);

		node = node->next;
	}

	return code;
}


size_t binary_reverse(size_t binary, size_t bits)
{
	size_t reversed = 0;

	size_t i;
	for (i = 0; i < bits; i++)
	{
		size_t temp = (binary & (1 << i));
		if (temp) reversed |= (1 << ((bits - 1) - i));
	}

	return reversed & ((1 << bits) - 1);
}

void binary_print(void* ptr, size_t length)
{
	unsigned char* p = (unsigned char*) ptr;
	char quad1[] = "0000";
	char quad2[] = "0000";

	int i, j;
	for (i = length - 1; i >= 0; i--)
	{
		for (j = 0; j < 4; j++)
		{
			if ((p[i] >> (j + 4)) & 1) quad1[3 - j] = '1';
			else quad1[3 - j] = '0';
		}

		for (j = 0; j < 4; j++)
		{
			if ((p[i] >> j) & 1) quad2[3 - j] = '1';
			else quad2[3 - j] = '0';
		}

		printf("[%s %s] ", quad1, quad2);
	}

	printf("\n");
}


void tree_print(node_t* node, size_t level)
{
	size_t i;

	if (node != NULL)
	{
		tree_print(node->right, level + 1);

		for (i = 1; i < level; i++) printf("       ");
		if (level) printf("%zu------", level - 1);
		if (node->next && node == node->next->left) printf("\\");
		else if (node->next && node == node->next->right) printf("/");

		printf("(%zu)", node->frequency);
		if (!node->left || !node->right) printf(" 0x%X %3d '%c'", node->symbol, node->symbol, node->symbol);
		printf("\n");

		tree_print(node->left, level + 1);
	}
}

node_t* tree_node(unsigned char symbol, size_t frequency, node_t* next, node_t* left, node_t* right)
{
	node_t* node = (node_t*) malloc(sizeof(node_t));
	node->symbol = symbol;
	node->frequency = frequency;
	node->next = next;
	node->left = left;
	node->right = right;
	return node;
}

void tree_free(node_t* node)
{
	if (node)
	{
		tree_free(node->left);
		tree_free(node->right);
		free(node);
	}
}

void list_print(node_t* head)
{
	while (head)
	{
		tree_print(head, 0);
		head = head->next;
	}
}

node_t* list_insert_sorted(node_t* head, node_t* tail)
{
	if (!tail) return head;

	if (!head || tail->frequency < head->frequency)
	{
		tail->next = head;
		head = tail;
	}
	else
	{
		node_t* rover = head;
		node_t* next = head->next;

		while (rover)
		{
			if (!next || tail->frequency < next->frequency)
			{
				rover->next = tail;
				tail->next = next;
				break;
			}

			rover = next;
			next = next->next;
		}
	}

	return head;
}
