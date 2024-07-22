#ifndef COMPRESSOR_UTILS
#define COMPRESSOR_UTILS

#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <filesystem>
#include <iostream>
#include <vector>
#include <cstring>

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

  task(ulong size, u_char* data) {
    char flag;
    ulong filename_size;
    ulong ptr = 0;
    memcpy(&flag, data + ptr, 1);
    ptr += 1;
    memcpy(&filename_size, data + ptr, sizeof(ulong));
    ptr += sizeof(ulong);
    auto filename = new char[filename_size];
    memcpy(filename, data + ptr, filename_size);
    ptr += filename_size;
    this->filename = std::string(filename, filename_size);
    delete[] filename;
    ulong d_start;
    memcpy(&d_start, data + ptr, sizeof(ulong));
    this->d_start = d_start;
    ptr += sizeof(ulong);
    ulong d_size;
    memcpy(&d_size, data + ptr, sizeof(ulong));
    this->d_size = d_size;
    ptr += sizeof(ulong);
    if (flag == 0) {
      // this->d_data = new u_char[size-ptr];
      // memcpy(this->d_data, data + ptr, size - ptr);
      this->d_data = data + ptr;
      this->decompress = false;
    } else {
      ulong c_size;
      memcpy(&c_size, data + ptr, sizeof(ulong));
      this->c_size = c_size;
      ptr += sizeof(ulong);
      // this->c_data = new u_char[size-ptr];
      // memcpy(this->c_data, data + ptr, size - ptr);
      this->c_data = data + ptr;
      this->decompress = true;
    }
  }

  // Define delete operator
  ~task() {
    if (this->dyn_d_data && this->d_data != nullptr) {
      delete[] this->d_data;
    }
    if (this->dyn_c_data && this->c_data != nullptr) {
      delete[] this->c_data;
    }
  }

  std::string filename;
  bool decompress;

  ulong d_start;
  ulong d_size;
  unsigned char* d_data;

  ulong c_size;
  unsigned char* c_data;

  std::pair<ulong, u_char*> to_data() {
    if (this->decompress) {
      return this->to_decompress_data();
    } else {
      return this->to_compress_data();
    }
  }

  void set_d_data(unsigned char* d_data) {
    this->d_data = d_data;
    this->dyn_d_data = true;
  }

  void set_c_data(unsigned char* c_data) {
    this->c_data = c_data;
    this->dyn_c_data = true;
  }


  private:
    bool dyn_d_data = false;
    bool dyn_c_data = false;

    std::pair<ulong, u_char*> to_compress_data() {
      char zero = 0;
      ulong filename_size = this->filename.size();
      ulong len = 
        1 +                       // data flag
        sizeof(ulong) +           // filename size
        filename_size +           // filename
        sizeof(ulong) * 2 +       // d_start, d_size
        this->d_size;             // d_data
      auto data = new u_char[len];
      ulong ptr = 0;
      memcpy(data + ptr, &zero, 1);
      ptr += 1;
      memcpy(data + ptr, &filename_size, sizeof(ulong));
      ptr += sizeof(ulong);
      memcpy(data + ptr, this->filename.c_str(), filename_size);
      ptr += filename_size;
      memcpy(data + ptr, &(this->d_start), sizeof(ulong));
      ptr += sizeof(ulong);
      memcpy(data + ptr, &(this->d_size), sizeof(ulong));
      ptr += sizeof(ulong);
      memcpy(data + ptr, this->d_data, this->d_size);

      return std::pair<ulong, u_char*>(len, data);
    }

    std::pair<ulong, u_char*> to_decompress_data() {
      char one = 1;
      ulong filename_size = this->filename.size();
      ulong len = 
        1 +                       // data flag
        sizeof(ulong) +           // filename size
        filename_size +           // filename
        sizeof(ulong) * 3 +       // d_start, d_size, c_size
        this->c_size;             // c_data
      auto data = new u_char[len];
      ulong ptr = 0;
      memcpy(data + ptr, &one, 1);
      ptr += 1;
      memcpy(data + ptr, &filename_size, sizeof(ulong));
      ptr += sizeof(ulong);
      memcpy(data + ptr, this->filename.c_str(), filename_size);
      ptr += filename_size;
      memcpy(data + ptr, &(this->d_start), sizeof(ulong));
      ptr += sizeof(ulong);
      memcpy(data + ptr, &(this->d_size), sizeof(ulong));
      ptr += sizeof(ulong);
      memcpy(data + ptr, &(this->c_size), sizeof(ulong));
      ptr += sizeof(ulong);
      memcpy(data + ptr, this->c_data, this->c_size);

      return std::pair<ulong, u_char*>(len, data);
    }
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
    Entity() {};

    std::vector<File> get_files() { return this->files; };

  private:
    bool exists = false;
    std::vector<File> files = {};
};

// Other

bool writeFile(const std::string &filename, unsigned char *ptr, size_t size);
bool writeFileTo(const std::string &filename, ulong to, unsigned char *ptr, size_t size);
bool writeFileEnd(const std::string &filename, unsigned char *ptr, size_t size);
bool removeFile(File file);

#endif