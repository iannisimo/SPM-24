#ifndef COMPRESSOR_SEQUENTIAL
#define COMPRESSOR_SEQUENTIAL

#include "utils.hpp"
#include "logger.hpp"
#include "miniz.h"

bool s_compressFile(File file, char* suff, bool keep, int split_size);
bool s_decompressFile(File file, char* suff, bool keep);

#endif