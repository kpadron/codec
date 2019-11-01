#include "chacha.h"
#include "codec.h"

int main(int argc, char** argv)
{
    return codec_main(argc, argv, "ChaCha", ".chacha", chacha_encode_filepath, chacha_decode_filepath);
}
