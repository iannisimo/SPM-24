#ifndef _COMPRESSOR_UTILS
#define _COMPRESSOR_UTILS

#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <filesystem>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

/**
 * \brief Keep info and pointers about a single file
 * \param path: std::filesystem::path to the file
 * 
 * 
*/
class File {
  public:
    File(fs::path path);

    std::string get_name() { return this->path.filename(); };
    std::string get_abs_path() { return fs::canonical(this->path); };
    bool exists() { return fs::exists(this->path); };
    operator bool() { return this->exists();};

    bool load();

  private:
    fs::path path;
    struct stat stats;
    unsigned char* contents;

    int get_magic(unsigned short len, char* buf);
    bool is_compressed();
};

/**
 * \brief Store information about an entity to [de]compress (be it a single file or a directory)
 * \param rel_path: relative path to the entity
*/
class Entity {
  public:
    std::string name;
    std::string abs_path;

    operator bool() const;
    Entity(std::string rel_path);
    Entity(char* rel_path);

    std::vector<File> get_files() { return this->files; };

  private:
    bool exists = false;
    std::vector<File> files;
};

#endif