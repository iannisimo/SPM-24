#ifndef COMPRESSOR_FASTFLOW
#define COMPRESSOR_FASTFLOW

#include "utils.hpp"
#include "logger.hpp"
#include "miniz.h"
#include "ff/ff.hpp"

struct Source : ff::ff_monode_t<task> {
    Source(std::vector<Entity> entities, bool decompress, std::string suff, bool keep, ulong split_size);
    task* svc(task* input);

    std::vector<Entity> entities;
    bool decompress;
    std::string suff;
    bool keep;
    ulong split_size;
};

struct Worker : ff::ff_minode_t<task> {
  task* svc(task* input);
};

struct Sink : ff::ff_minode_t<task> {
  task* svc(task* input);
};

bool work(std::vector<Entity> entities, bool decompress, std::string suff, bool keep, ulong split_size, int n_threads);


#endif