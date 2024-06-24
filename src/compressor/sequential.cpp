#include "miniz.h"
#include "utils.hpp"
#include "sequential.hpp"
#include "logger.hpp"
#include <format>

bool s_compressFile(File file, char* suff, bool keep, ulong split_size) {
  std::vector<ulong> splits = file.get_splits(split_size);
  ulong tot_bound = ZIP_MAGIC_LEN     // space for magic bytes
    + sizeof(ulong)                   // space for the length of the uncompressed file
    + sizeof(ulong)                   // space for the upper-bound of the uncompressed splits
    + splits.size() * sizeof(ulong);  // space for the size of each split (compressed)
    
  for (int idx = 0; idx < (int) splits.size() - 1; idx++) {
    ulong start = splits[idx];
    ulong end = splits[idx + 1];
    tot_bound += 2 * sizeof(ulong);
    tot_bound += mz_compressBound(end - start);
  }

  auto dest = new unsigned char[tot_bound];
  memcpy(dest, ZIP_MAGIC, ZIP_MAGIC_LEN); // Write the magic bytes to the buffer

  ulong tot_written = ZIP_MAGIC_LEN 
    + sizeof(ulong)
    + sizeof(ulong)
    + splits.size() * sizeof(ulong);

  
  size_t file_size = file.size();
  memcpy(dest + ZIP_MAGIC_LEN, &file_size, sizeof(ulong));

  memcpy(dest + ZIP_MAGIC_LEN + sizeof(ulong), &split_size, sizeof(ulong));

  for (ulong idx = 0; idx < (ulong) splits.size() - 1; idx++) {
    memcpy(dest + ZIP_MAGIC_LEN + (2 + idx) * sizeof(ulong), &tot_written, sizeof(ulong));
    ulong start = splits[idx];
    ulong end = splits[idx + 1];
    ulong cmp_bound = mz_compressBound(end - start);

    memcpy((dest + tot_written), &start, sizeof(ulong));
    memcpy((dest + tot_written + sizeof(ulong)), &end, sizeof(ulong));

    tot_written += 2 * sizeof(ulong);

    int ret;
    uint32_t size = ((uint32_t) ((end - start)));
    if ((ret = mz_compress((dest + tot_written), &cmp_bound, (const unsigned char *) (file.contents + start), size)) != MZ_OK) {
      LOG_E("miniz", "Error compressing file");
      return false;
    }
    tot_written += cmp_bound;
  }
  memcpy(dest + ZIP_MAGIC_LEN + (2 + splits.size() - 1) * sizeof(ulong), &tot_written, sizeof(ulong));

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
  std::vector<ulong> splits = file.get_splits(0);

  ulong filesize;
  memcpy(&filesize, file.contents + ZIP_MAGIC_LEN, sizeof(ulong));

  ulong uncompressed_bound; // Retreive the max split_size from the compressed file
  memcpy(&uncompressed_bound, file.contents + ZIP_MAGIC_LEN + sizeof(ulong), sizeof(ulong));
  
  // ulong tot_bound = uncompressed_bound * (splits.size() - 1);
  auto dest = new unsigned char[filesize];

  ulong tot_read = 0;

  for (ulong idx = 0; idx < (ulong) splits.size() - 1; idx++) {
    ulong start = splits[idx];
    ulong end = splits[idx + 1];
    mz_ulong bound = uncompressed_bound;

    ulong u_start, u_end;
    memcpy(&u_start, file.contents + start, sizeof(ulong));
    memcpy(&u_end, file.contents + start + sizeof(ulong), sizeof(ulong));
    int ret;
    // LOG_D("decompress", std::format("start: {}, end: {}, size: {}, tot_read: {}", start, end, filesize, tot_read));
    if ((ret = mz_uncompress(dest + u_start, &bound, (const unsigned char*) (file.contents + start + 2 * sizeof(ulong)), end - start)) != MZ_OK) {
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

bool s_work(std::vector<Entity> entities, bool decompress, char * suff, bool keep, ulong split_size) {
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

      std::vector<ulong> splits = f.get_splits(split_size);

      if (decompress) {
        s_decompressFile(f, suff, keep);
      } else {
        s_compressFile(f, suff, keep, split_size);
      }
    }
  }
  return true;
}
