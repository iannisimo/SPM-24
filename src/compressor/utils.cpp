
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <string.h>

const char* ZIP_MAGIC = "spmzip";
const unsigned char ZIP_MAGIC_LEN = 6;


namespace fs = std::filesystem;

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

File::File(fs::path path) {
  this->path = path;
  std::cout << this->path.filename() << std::endl;
  this->load();
}

void File::load() {
  std::cout << this->is_compressed() << std::endl;
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
