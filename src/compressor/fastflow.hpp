#ifndef COMPRESSOR_FASTFLOW
#define COMPRESSOR_FASTFLOW

#include "utils.hpp"
#include "logger.hpp"
#include "miniz.h"

bool f_compressFile(File file, char* suff, bool keep, int split_size);
bool f_decompressFile(File file, char* suff, bool keep);

bool f_work(std::vector<Entity> entities, bool decompress, char* suff, bool keep, int split_size);


#endif