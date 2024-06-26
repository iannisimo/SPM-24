#ifndef COMPRESSOR_FASTFLOW
#define COMPRESSOR_FASTFLOW

#include "utils.hpp"
#include "logger.hpp"
#include "miniz.h"
#include "ff/ff.hpp"

struct task {
  task(std::string filename, ulong d_start, ulong d_end, unsigned char* d_data) : 
    filename(filename), d_start(d_start), d_end(d_end), d_data(d_data) {
      this->decompress = false;
    };
  task(std::string filename, ulong c_size, unsigned char* c_data) :
    filename(filename), c_size(c_size), c_data(c_data) {
      this->decompress = true;
    };

  std::string filename;
  bool decompress;

  ulong d_start;
  ulong d_end;
  unsigned char* d_data;

  ulong c_size;
  unsigned char* c_data;
};

struct SourceSink : ff::ff_monode_t<task> {
    SourceSink(std::vector<Entity> entities, bool decompress, char* suff, bool keep, ulong split_size);
    task* svc(task* input);

    std::vector<Entity> entities;
    bool decompress;
    char* suff;
    bool keep;
    ulong split_size;
};

struct Worker : ff::ff_node_t<task> {
  task* svc(task* input);
};



bool f_compressFile(File file, char* suff, bool keep, int split_size);
bool f_decompressFile(File file, char* suff, bool keep);

bool f_work(std::vector<Entity> entities, bool decompress, char* suff, bool keep, ulong split_size, int n_threads);


#endif