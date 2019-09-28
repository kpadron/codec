#pragma once
#include "utility.h"

typedef buffer_t (*file_hasher_t)(const char*);

int hash_main(int argc, char** argv, const char* hash_name, file_hasher_t hash_func);
