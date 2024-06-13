
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <utility>
#include <fcntl.h>
#include "logger.hpp"
#include <sys/mman.h>
#include <unistd.h>

const char* ZIP_MAGIC = "spmzip";
const unsigned char ZIP_MAGIC_LEN = 6;


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
    std::cout << this->name << " is a directory" << std::endl;
    for (auto &&elem : fs::recursive_directory_iterator(p)) {
      if (elem.is_regular_file()) {
        this->files.emplace_back(File(elem));
      }
    }
    
  } else {

    perror("file type");
    std::cerr << this->name << " is not a file nor a directory" << std::endl;
    exit(1);

  }
}

Entity::Entity(char* rel_path) : Entity::Entity(std::string(rel_path)) {}

// --------
// | File |
// --------

File::File(fs::path path) {
  this->path = std::move(path);
  LOG_D("File", "Created file " + this->get_name());
  this->load();
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
    return false;
  }

  this->contents = (unsigned char*) mmap(nullptr, this->stats.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (this->contents == MAP_FAILED) {
    LOG_E("mmap", "Error mapping file to memory: " + this->get_name());
  }
  close(fd);

  std::cout << this->contents << std::endl;

  return true;
}

int File::get_magic(unsigned short len, char* buf) {
  if (!this->exists()) return 0;
  std::ifstream file(this->get_abs_path(), std::ios::binary | std::ios::in);
  if (!file.is_open()) {
    perror("file open");
    std::cerr << "Could not open " << this->get_abs_path() << std::endl;
    return -1;
  }
  file.read(buf, len);
  file.close();
  return 0;
}

bool File::is_compressed() {
  char buf[ZIP_MAGIC_LEN];
  if (this->get_magic(ZIP_MAGIC_LEN, buf)) {
    perror("magic");
    std::cerr << "Could not read magic for file " << this->get_abs_path() << std::endl;
  }
  if (ZIP_MAGIC == std::string(buf)) {
    return true;
  }
  return false;
}
