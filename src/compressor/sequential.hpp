#ifndef COMPRESSOR_SEQUENTIAL
#define COMPRESSOR_SEQUENTIAL

#include "utils.hpp"
#include "logger.hpp"
#include "miniz.h"

bool s_compressFile(File file, char* suff, bool keep, ulong split_size);
bool s_decompressFile(File file, char* suff, bool keep);

bool s_work(std::vector<Entity> entities, bool decompress, char* suff, bool keep, ulong split_size);

#endif