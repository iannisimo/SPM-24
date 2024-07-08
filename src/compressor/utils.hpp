#ifndef COMPRESSOR_UTILS
#define COMPRESSOR_UTILS

#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <filesystem>
#include <iostream>
#include <vector>

const char ZIP_MAGIC[] = "spmzip";
const unsigned char ZIP_MAGIC_LEN = 6;

struct split {
  split() {
    data = nullptr;
    start = size = 0;
  };

  split(u_char* data, ulong start, ulong size) : data(data), start(start), size(size) {};

  u_char* data;
  ulong start;
  ulong size;
};


struct task {
  task(std::string filename, ulong d_start, ulong d_size, unsigned char* d_data) : 
    filename(filename), d_start(d_start), d_size(d_size), d_data(d_data) {
      this->decompress = false;
    };
  task(std::string filename, ulong d_start, ulong d_size, ulong c_size, unsigned char* c_data) :
    filename(filename), d_start(d_start), d_size(d_size), c_size(c_size), c_data(c_data) {
      this->decompress = true;
    };
  task() {};

  std::string filename;
  bool decompress;

  ulong d_start;
  ulong d_size;
  unsigned char* d_data;

  ulong c_size;
  unsigned char* c_data;
};


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
    File() {};

    std::string get_name() { return this->path.filename(); };
    std::string get_abs_path() { return fs::canonical(this->path); };
    bool exists() { return fs::exists(this->path); };
    operator bool() { return this->exists();};
    ulong size() { return (ulong) this->stats.st_size; };
    bool uncompressed_size(ulong *size);
    bool max_split(ulong *size);

    bool load();
    bool unload();
    bool is_compressed();


    split get_split(ulong split_size);

    ulong get_splits_ub(ulong split_size);

    std::string get_out_path(std::string suff);

    unsigned char* contents = nullptr;

  private:
    fs::path path;
    char compressed = -1;
    ulong next_split = 0;
    struct stat stats;
    ulong _uncompressed_size = 0;
    ulong _max_split = 0;

    bool get_magic(char* buf);
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
    Entity(std::string rel_path, bool recurse);
    Entity(char* rel_path, bool recurse);

    std::vector<File> get_files() { return this->files; };

  private:
    bool exists = false;
    std::vector<File> files = {};
};

// Other

bool writeFile(const std::string &filename, unsigned char *ptr, size_t size);
bool writeFileTo(const std::string &filename, ulong to, unsigned char *ptr, size_t size);
bool writeFileEnd(const std::string &filename, unsigned char *ptr, size_t size);
bool writeC(const std::string &filename, ulong ci);
bool removeFile(File file);

#endif