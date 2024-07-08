
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

Entity::Entity(std::string rel_path, bool recurse) {
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
    if (recurse) {
      // Recurse folder and add all files to `this`
      LOG_I("entity", std::format("{} is a directory... recursing", this->name));
      for (auto &&elem : fs::recursive_directory_iterator(p)) {
        if (elem.is_regular_file()) {
          this->files.emplace_back(File(elem));
        }
      }
    } else {
      LOG_I("entity", std::format("{} is a directory... skipping", this->name));
    }
    
  } else {

    LOG_E("entity", std::format("{} is not a file nor a directory", this->name));
    return;

  }
}

Entity::Entity(char* rel_path, bool recurse) : Entity::Entity(std::string(rel_path), recurse) {}

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

  this->contents = (unsigned char*) mmap(nullptr, this->size(), PROT_READ, MAP_PRIVATE, fd, 0);
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
  if (munmap(this->contents, this->size()) < 0) {
    LOG_E("file unload", std::format("Could not unload {}", this->get_name()));
    return false;
  }
  return true;
}

bool File::uncompressed_size(ulong *size) {
  if (this->_uncompressed_size == 0) {
    if (!this->exists()) return false;
    if (!this->is_compressed()) {
      this->_uncompressed_size = this->size();
    } else {
      if (this->contents == nullptr) {
        LOG_D("uncompressed_size", "The file needs to be loaded beforehand");
        return false;
      }
      memcpy(&(this->_uncompressed_size), this->contents + ZIP_MAGIC_LEN, sizeof(ulong));
    }
  }
  *size = this->_uncompressed_size;
  return true;
}

bool File::max_split(ulong *size) {
  if (this->_max_split == 0) {
    if (!this->exists()) return false;
    if (!this->is_compressed()) return false;
    if (this->contents == nullptr) {
      LOG_D("max_split", "The file needs to be loaded beforehand");
      return false;
    }
    memcpy(&(this->_max_split), this->contents + ZIP_MAGIC_LEN + sizeof(ulong), sizeof(ulong));
  }
  *size = this->_max_split;
  return true;
}

split File::get_split(ulong split_size)
{
  if (this->contents == nullptr) {
    if (!this->load()) {
      LOG_D("get_split", std::format("Could not load the file"));
      return split();
    }
  }
  if (!this->is_compressed()) {
    if (this->next_split >= this->size()) {
      return split();
    }
    if (this->size() <= split_size) {
      this->next_split = this->size();
      return split(this->contents, 0, this->size());
    }
    ulong split_from = this->next_split;
    this->next_split += split_size;
    if (this->next_split > this->size()) {
      this->next_split = this->size();
    }
    return split(this->contents + split_from, split_from, this->next_split - split_from);
  }
  // is_compressed==true
  if (this->next_split == 0) this->next_split = ZIP_MAGIC_LEN + 2*sizeof(ulong);
  if (this->next_split >= this->size()) {
    return split();
  }
  ulong split_from = this->next_split;
  ulong _split_size, _split_start;
  memcpy(&_split_size, this->contents + split_from, sizeof(ulong));
  memcpy(&_split_start, this->contents + split_from + sizeof(ulong), sizeof(ulong));
  this->next_split += _split_size;
  _split_size -= 2 * sizeof(ulong);
  return split(this->contents + split_from + 2*sizeof(ulong), _split_start, _split_size);
}

ulong File::get_splits_ub(ulong split_size) {
  return (this->size() / split_size) + 1;
}

bool File::get_magic(char *buf) {
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

std::string File::get_out_path(std::string suff) {
  if (this->is_compressed() && this->get_name().ends_with(suff)) {
    return this->get_abs_path().substr(0, this->get_abs_path().size() - suff.size());
  }
  return this->get_abs_path().append(suff);
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
