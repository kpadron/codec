#pragma once
#include "utility.h"

// Encode data using RLE codec and place into provided buffer
int rle_encode(const void* data, size_t size, void* encoded_data, size_t* encoded_size);

// Decode data using RLE codec and place into provided buffer
int rle_decode(const void* data, size_t size, void* decoded_data, size_t* decoded_size);

// Encode data using RLE codec and return a buffer
buffer_t rle_encode_buffer(const buffer_t buffer);

// Decode data using RLE codec and return a buffer
buffer_t rle_decode_buffer(const buffer_t buffer);

// Encode file using the RLE codec
void rle_encode_filepath(const char* inpath, const char* outpath);

// Decode file using the RLE codec
void rle_decode_filepath(const char* inpath, const char* outpath);
