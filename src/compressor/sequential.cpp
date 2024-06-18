#include "miniz.h"
#include "utils.hpp"
#include "sequential.hpp"
#include "logger.hpp"
#include <format>

bool s_compressFile(File file, char* suff, bool keep, int split_size) {
  std::vector<int> splits = file.get_splits(split_size);
  int tot_bound = ZIP_MAGIC_LEN   // space for magic bytes
    + splits.size() * sizeof(int) // space for the size of each split (compressed)
    + sizeof(int);                // space for the upper-bound of the uncompressed splits
  for (int idx = 0; idx < (int) splits.size() - 1; idx++) {
    unsigned long start = splits[idx];
    unsigned long end = splits[idx + 1];
    tot_bound += mz_compressBound(end - start);
  }
  auto dest = new unsigned char[tot_bound];
  memcpy(dest, ZIP_MAGIC, ZIP_MAGIC_LEN);
  int tot_written = ZIP_MAGIC_LEN 
    + splits.size() * sizeof(int)
    + sizeof(int);
  memcpy(dest + ZIP_MAGIC_LEN + splits.size() * sizeof(int), &split_size, sizeof(int));
  for (int idx = 0; idx < (int) splits.size() - 1; idx++) {
    memcpy(dest + ZIP_MAGIC_LEN + idx * sizeof(int), &tot_written, sizeof(int));
    LOG_D("", std::format("Writing {} at {}", tot_written, ZIP_MAGIC_LEN + idx * sizeof(int)));
    unsigned long start = splits[idx];
    unsigned long end = splits[idx + 1];
    unsigned long cmp_bound = mz_compressBound(end - start);

    int ret;
    uint32_t size = ((uint32_t) ((end - start)));
    if ((ret = mz_compress((dest + tot_written), &cmp_bound, (const unsigned char *) (file.contents + start), size)) != MZ_OK) {
      LOG_E("miniz", "Error compressing file");
      return false;
    }
    tot_written += cmp_bound;
  }
  memcpy(dest + ZIP_MAGIC_LEN + (splits.size() - 1) * sizeof(int), &tot_written, sizeof(int));

  if (!writeFile(std::format("{}{}", file.get_abs_path(), suff), dest, tot_written)) {
    LOG_E("compress", "Could not write compressed file");
    return false;
  }

  delete[] dest;

  if (!keep) {
    return removeFile(file);
  }
    
  return true;
}

bool s_decompressFile(File file, char* suff, bool keep) {
  std::vector<int> splits = file.get_splits(0);
  int uncompressed_bound; // Retreive the max split_size from the compressed file
  memcpy(&uncompressed_bound, file.contents + ZIP_MAGIC_LEN + splits.size() * sizeof(int), sizeof(int));
  
  int tot_bound = uncompressed_bound * (splits.size() - 1);
  auto dest = new unsigned char[tot_bound];

  int tot_read = 0;

  for (int idx = 0; idx < (int) splits.size() - 1; idx++) {
    unsigned long start = splits[idx];
    unsigned long end = splits[idx + 1];
    mz_ulong bound = uncompressed_bound;
    int ret;
    LOG_D("decompress", std::format("start: {}, end: {}, bound: {}, tot_read: {}", start, end, tot_bound, tot_read));
    if ((ret = mz_uncompress(dest + tot_read, &bound, (const unsigned char*) (file.contents + start), end - start)) != MZ_OK) {
      LOG_E("miniz", std::format("Error uncompressing file: {}", ret));
      return false;
    }
    tot_read += bound;
  }

  std::string filename;
  if (file.get_name().ends_with(suff)) {
    filename = file.get_abs_path().substr(0, file.get_abs_path().size() - std::string(suff).size());
  } else {
    filename = file.get_abs_path().append(suff);
  }

  if (!writeFile(filename, dest, tot_read)) {
    LOG_E("decompress", "Could not write decompressed file");
    return false;
  }

  delete[] dest;

  if (!keep) {
    return removeFile(file);
  }

  return true;
}