#include "miniz.h"
#include "utils.hpp"
#include "sequential.hpp"
#include "logger.hpp"
#include <format>

bool s_compressFile(File file, std::string suff, ulong split_size) {
  ulong tot_bound = ZIP_MAGIC_LEN     // space for magic bytes
    + sizeof(ulong)                   // space for the length of the uncompressed file
    + sizeof(ulong);                  // space for the upper-bound of the uncompressed splits

  ulong n_splits = file.get_splits_ub(split_size);
  ulong s_bound = mz_compressBound(split_size);
  tot_bound += n_splits * (s_bound + 2 * sizeof(ulong));

  auto dest = new unsigned char[tot_bound];


  memcpy(dest, ZIP_MAGIC, ZIP_MAGIC_LEN); // Write the magic bytes to the buffer
  ulong tot_written = ZIP_MAGIC_LEN;
  
  size_t file_size = file.size();
  memcpy(dest + tot_written, &file_size, sizeof(ulong));
  tot_written += sizeof(ulong);
  
  memcpy(dest + tot_written, &split_size, sizeof(ulong));
  tot_written += sizeof(ulong);


  split s;
  while ((s = file.get_split(split_size)).data != NULL) {

    u_char* sizePtr = dest + tot_written;
    tot_written += sizeof(ulong);
    memcpy(dest + tot_written, &(s.start), sizeof(ulong));
    tot_written += sizeof(ulong);
    ulong cmp_bound = mz_compressBound(s.size);
    int ret;
    if ((ret = mz_compress((dest + tot_written), &cmp_bound, s.data, s.size)) != MZ_OK) {
      LOG_E("miniz", std::format("Error compressing file: {}", ret));
      return false;
    }
    ulong chunksize = cmp_bound + 2*sizeof(ulong);
    memcpy(sizePtr, &(chunksize), sizeof(ulong));
    tot_written += cmp_bound;
  }

  if (!writeFile(file.get_out_path(suff), dest, tot_written)) {
    LOG_E("compress", "Could not write compressed file");
    return false;
  }

  delete[] dest;
    
  return true;
}

bool s_decompressFile(File file, std::string suff) {
  ulong filesize, max_bound;
  if (!file.uncompressed_size(&filesize)) return false;
  if (!file.max_split(&max_bound)) return false;
  auto dest = new u_char[filesize];

  split s;
  while ((s = file.get_split(0)).data != NULL) {
    LOG_D("AAA", std::format("{} {}", s.size, s.start));
    ulong bound = max_bound;
    int ret;
    if ((ret = mz_uncompress(dest + s.start, &bound, s.data, s.size)) != MZ_OK) {
      LOG_E("miniz", std::format("Error uncompressing file: {}", ret));
      return false;
    }
  }

  if (!writeFile(file.get_out_path(suff), dest, filesize)) {
    LOG_E("decompress", "Could not write decompressed file");
    return false;
  }

  delete[] dest;

  return true;
}

bool work(std::vector<Entity> entities, bool decompress, std::string suff, ulong split_size) {
  for (auto &&e : entities) {
    for (auto &&f : e.get_files()) {
      LOG_D("main", std::format("Processing file `{}`", f.get_name()));

      // Skip files which are already [de]compressed
      bool compressed = f.is_compressed();
      if (compressed != decompress) continue;

      if (!f.load()) {
        LOG_E("Files", "\tCould not load file");
        continue;
      }

      LOG_D("compressed", std::format("{}", f.is_compressed()));

      if (decompress) {
        s_decompressFile(f, suff);
      } else {
        s_compressFile(f, suff, split_size);
      }
    }
  }
  return true;
}
