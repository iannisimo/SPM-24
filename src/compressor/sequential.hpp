#ifndef COMPRESSOR_SEQUENTIAL
#define COMPRESSOR_SEQUENTIAL

#include "utils.hpp"
#include "logger.hpp"
#include "miniz.h"

char done = 1;

int mz_compress_(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len) {
  extern char done;
  LOG_D("mz_compress", "");
  *pDest_len = source_len / 2;
  for (ulong i = 0; i < *pDest_len; i++) {
    pDest[i] = done;
  }
  done += 1;
  return MZ_OK;
}

bool s_compressFile(File file, std::string suff, bool keep, ulong split_size);
bool s_decompressFile(File file, std::string suff, bool keep);

bool s_work(std::vector<Entity> entities, bool decompress, std::string suff, bool keep, ulong split_size);

#endif