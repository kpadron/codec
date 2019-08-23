#include "miniz.h"
#include <stdio.h>

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Usage: INFILE [OUTFILE]\n");
		return 0;
	}

	const char* const inpath = argv[1];
	const char* outpath = inpath;

	if (argc > 2)
		outpath = argv[2];

	FILE* f = fopen(inpath, "rb");
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	uint8_t* data = (uint8_t*) malloc(size);
	fread(data, size, 1, f);
	fclose(f);

	uint32_t crc = tinfl_crc32(MZ_CRC32_INIT, data, size);
	uint32_t adler = tinfl_adler32(MZ_ADLER32_INIT, data, size);

	printf("%s (%zu)|\n", inpath, size);
	printf(" crc: %u 0x%X\n", crc, crc);
	printf(" adler: %u 0x%X\n", adler, adler);

	const int flags = TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_COMPUTE_ADLER32;
	size_t csize = 0;
	uint8_t* cdata = (uint8_t*) tinfl_decompress_mem_to_heap(data, size, &csize, flags);

	if (cdata == NULL)
	{
		printf("Failed to decompress file '%s'\n", inpath);
		return -1;
	}

	crc = tinfl_crc32(MZ_CRC32_INIT, cdata, csize);
	adler = tinfl_adler32(MZ_ADLER32_INIT, cdata, csize);

	printf("%s (%zu)|\n", outpath, csize);
	printf(" crc: %u 0x%X\n", crc, crc);
	printf(" adler: %u 0x%X\n", adler, adler);

	f = fopen(outpath, "wb");
	fwrite(cdata, csize, 1, f);
	fclose(f);

	return 0;
}
