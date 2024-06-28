
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <utility>
#include <fcntl.h>
#include "logger.hpp"
#include <sys/mman.h>
#include <unistd.h>
#include <format>


namespace fs = std::filesystem;

// ----------
// | Entity |
// ----------

Entity::operator bool() const {
  return this->exists;
}

Entity::Entity(std::string rel_path) {
  // Strip the trailing `/`
  rel_path = rel_path.ends_with("/") ? rel_path.substr(0, rel_path.length() - 1) : rel_path;
  // Extract path info from std::filesystem
  fs::path p(rel_path);
  this->exists = fs::exists(p);
  if (!this->exists) {
    return;
  }
  this->name = p.filename();
  this->abs_path = fs::canonical(p);
  
  // Check if `this` is a single file or a directory
  if (fs::is_regular_file(p)) {
    
    // Add file to `this`
    this->files.emplace_back(File(p));

  } else if (fs::is_directory(p)) {
    
    // Recurse folder and add all files to `this`
    LOG_D("entity", std::format("{} is a directory... recursing", this->name));
    for (auto &&elem : fs::recursive_directory_iterator(p)) {
      if (elem.is_regular_file()) {
        this->files.emplace_back(File(elem));
      }
    }
    
  } else {

    LOG_E("entity", std::format("{} is not a file nor a directory", this->name));
    return;

  }
}

Entity::Entity(char* rel_path) : Entity::Entity(std::string(rel_path)) {}

// --------
// | File |
// --------

File::File(fs::path path) {
  this->path = std::move(path);
}

bool File::load() {
  if (!this->exists()) {
    LOG_E("load", "File does not exist");
      return false;
  }

  int fd = open(this->get_abs_path().c_str(), O_RDONLY);

  if (fd < 0) {
    LOG_E("open", "Could not open file: " + this->get_name());
    return false;
  }

  if (fstat(fd, &this->stats)) {
    LOG_E("fstat", "Could not stat " + this->get_name());
    close(fd);
    return false;
  }

  this->contents = (unsigned char*) mmap(nullptr, this->stats.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (this->contents == MAP_FAILED) {
    LOG_E("mmap", "Error mapping file to memory: " + this->get_name());
    close(fd);
    return false;
  }
  // contents read successfully, closind fd
  close(fd);
  return true;
}

bool File::unload() {
  if (munmap(this->contents, this->stats.st_size) < 0) {
    LOG_E("file unload", std::format("Could not unload {}", this->get_name()));
    return false;
  }
  return true;
}

std::vector<ulong> File::get_splits(ulong split_size) {
  std::vector<ulong> splits = {};
  if (this->contents == nullptr) {
    LOG_D("get_splits", std::format("The file needs to be loaded in order to retreive the splits"));
    return splits;
  }
  // The file is not compressed
  if (!this->is_compressed()) {
    // file size is <= than the split size
    if (this->stats.st_size <= split_size) {
      splits.emplace_back(0);
      splits.emplace_back(this->stats.st_size);
      return splits;
    }
    // file size is > than the split size
    for (ulong s = 0; s < this->stats.st_size; s += split_size) {
      splits.emplace_back(s);
    }
    splits.emplace_back(this->stats.st_size);
    return splits;
  }
  // The file is compressed, read split sizes from header
  ulong split;
  ulong max_splits = this->stats.st_size / sizeof(ulong);
  for (int i = 0; i < max_splits; i++) {
    memcpy(&split, this->contents + ZIP_MAGIC_LEN + (2 + i) * sizeof(ulong), sizeof(ulong));
    if (!splits.empty() && split < splits.back()) {
      LOG_E("splits", std::format("The file `{}` is corrupt and cannot be decompressed", this->get_name()));
      std::vector<ulong> dummy = {};
      return dummy;
    }
    if (split == this->stats.st_size) {
      break;
    }
    splits.emplace_back(split);
  }
  splits.emplace_back(this->stats.st_size);
  return splits;
}

bool File::get_magic(char *buf)
{
  if (!this->exists()) return false;
  if (this->contents == nullptr) {
    std::ifstream file(this->get_abs_path(), std::ios::binary | std::ios::in);
    if (!file.is_open()) {
      LOG_E("get_magic", std::format("Could not open {}", this->get_name()));
      return false;
    }
    file.read(buf, ZIP_MAGIC_LEN);
    file.close();
  } else {
    memcpy(buf, this->contents, ZIP_MAGIC_LEN);
  }
  return true;
}

/**
 * \brief Check if a file is compressed by matching the first bytes to a signature
 * 
 * \return `true` if the signature matches; `false` otherwise
*/
bool File::is_compressed() {
  if (this->compressed < 0) {
    char buf[ZIP_MAGIC_LEN];
    if (!this->get_magic(buf)) {
      LOG_E("is_compressed", std::format("Could not read magic for file {}", this->get_name()));
      this->compressed = 0;
    }
    if (memcmp(buf, ZIP_MAGIC, ZIP_MAGIC_LEN) == 0) {
      this->compressed = 1;
    } else {
      this->compressed = 0;
    }
  }
  return this->compressed;
}

// ---------
// | Other |
// ---------

// write size bytes starting from ptr into filename
bool writeFile(const std::string &filename, unsigned char *ptr, size_t size) {
  FILE *pOutfile = fopen(filename.c_str(), "wb");
  if (!pOutfile) {
    LOG_E("writeFile", std::format("Failed opening output file {}", filename));
    return false;
  }
  if (ptr != nullptr) {
    if (fwrite(ptr, 1, size, pOutfile) != size) {
      LOG_E("writeFile", std::format("Failed writing to output file {}", filename));
      fclose(pOutfile);
      return false;
    }
  } else {
    if (fseek(pOutfile, size-1, SEEK_SET) != 0) {
      LOG_E("writeFile", std::format("Failed seeking file {}", filename));
      fclose(pOutfile);
      return false;
    }
    if (fwrite("", 1, 1, pOutfile) != 1) {
      LOG_E("writeFile", std::format("Failed writing last byte to output file {}", filename));
      fclose(pOutfile);
      return false;
    }
  }
  if (fclose(pOutfile) != 0)
    return false;
  return true;
}

bool writeFileTo(const std::string &filename, ulong to, unsigned char *ptr, size_t size) {
  FILE *pOutfile = fopen(filename.c_str(), "r+b");
  if (!pOutfile) {
    LOG_E("writeFileTo", std::format("Failed opening output file {}", filename));
    return false;
  }
  LOG_D("", std::format("{}: seeking to {}", filename, to));
  if (fseek(pOutfile, to, SEEK_SET) != 0) {
    LOG_E("writeFileTo", std::format("Failed seeking file {}", filename));
    fclose(pOutfile);
    return false;
  }
  if (fwrite(ptr, 1, size, pOutfile) != size) {
    LOG_E("writeFileTo", std::format("Failed writing to output file {}", filename));
    fclose(pOutfile);
    return false;
  }
  if (fclose(pOutfile) != 0)
    return false;
  return true;
}

bool writeFileEnd(const std::string &filename, unsigned char *ptr, size_t size) {
  FILE *pOutfile = fopen(filename.c_str(), "ab");
  if (!pOutfile) {
    LOG_E("writeFileTo", std::format("Failed opening output file {}", filename));
    return false;
  }
  if (fwrite(ptr, 1, size, pOutfile) != size) {
    LOG_E("writeFileTo", std::format("Failed writing to output file {}", filename));
    fclose(pOutfile);
    return false;
  }
  if (fclose(pOutfile) != 0)
    return false;
  return true;
}

bool writeC(const std::string &filename, ulong ci) {
  FILE *pOutfile = fopen(filename.c_str(), "r+b");
  if (!pOutfile) {
    LOG_E("writeFileTo", std::format("Failed opening output file {}", filename));
    return false;
  }
  
  // Skip magic bytes + 2 ulongs (file len, block size)
  if (fseek(pOutfile, ZIP_MAGIC_LEN + 2*sizeof(ulong), SEEK_SET) != 0) {
    LOG_E("writeFileTo", std::format("Failed seeking file {}", filename));
    fclose(pOutfile);
    return false;
  }
  ulong val, last_c;
  int pos = 0;
  do {
    pos += 1;
    last_c = val;
    if (fread(&val, sizeof(ulong), 1, pOutfile) != 1) {
      LOG_E("writeFileTo", std::format("Failed reading c for file {}", filename));
      fclose(pOutfile);
      return false;
    }
  } while (val != 0);
  ci += last_c;
  if (fseek(pOutfile, -sizeof(ulong), SEEK_CUR) != 0) {
    LOG_E("writeFileTo", std::format("Failed seeking file {}", filename));
    fclose(pOutfile);
    return false;
  }
  LOG_D("", std::format("Writing {} at {}", ci, pos));
  if (fwrite(&ci, sizeof(ulong), 1, pOutfile) != 1) {
    LOG_E("writeFileTo", std::format("Failed writing to output file {}", filename));
    fclose(pOutfile);
    return false;
  }
  if (fclose(pOutfile) != 0)
    return false;
  return true;
}

bool removeFile(File file) {
  if (unlink(file.get_abs_path().c_str()) == -1) {
    LOG_W("removeFile", std::format("Could not delete {}", file.get_name()));
    return false;
  }
  if (!file.unload()) {
    return false;
  }
  return true;
}
