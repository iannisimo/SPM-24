#ifndef COMPRESSOR_MPI
#define COMPRESSOR_MPI

#include "utils.hpp"
#include "logger.hpp"
#include "miniz.h"

class Scheduler {
  public:
    Scheduler(int size);

    bool has_free() {return n_free != 0;}

  private:
    int size;
    int n_free;
    std::vector<char> free;
};

bool EC_work(std::vector<Entity> entities, bool decompress, std::string suff, bool keep, ulong split_size, int n_threads);
bool W_work(int myId);


#endif