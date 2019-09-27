#pragma once
#include "utility.h"

typedef buffer_t (*file_hasher_t)(const char*);

int hash_main(int argc, char** argv, file_hasher_t hasher);
