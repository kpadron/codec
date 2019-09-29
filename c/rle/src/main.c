#include "rle.h"
#include "codec.h"

int main(int argc, char** argv)
{
    return codec_main(argc, argv, "RLE", ".rle", rle_encode_filepath, rle_decode_filepath);
}
