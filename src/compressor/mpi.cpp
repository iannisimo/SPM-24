#include "mpi.hpp"
#include "utils.hpp"
#include <format>
#include <filesystem>
#include <sys/time.h>
#include "logger.hpp"
#include <mpi.h>

bool EC_work(std::vector<Entity> entities, bool decompress, std::string suff, bool keep, ulong split_size, int n_threads) {
  int numProcs;
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
  Scheduler s(numProcs - 1);
  int entity = 0;
  int file = 0;
  LOG_D("EC", "Hello");

  return true;
}

bool W_work(int myId) {
  LOG_D(std::format("W_{}", myId), "Hello");
  return true;
}

Scheduler::Scheduler(int size) : size(size) {
  
  this->n_free = size;
}
