#include "cui/argparse.hpp"
#include "cui/usage.hpp"
#include "compressor/utils.hpp"
#include "compressor/mpi.hpp"
#include "logger.hpp"
#include <format>
#include "miniz.h"
#include <string>
#include <mpi.h>

int main(int argc, char *argv[]) {

	int myId, numProcs, nameLen;
	char processorName[MPI_MAX_PROCESSOR_NAME];
  
  MPI_Init(&argc, &argv);
  config args;
  args.argparse(argc, argv);
  if (args.help) {
    usage(args.pname);
    MPI_Finalize();
    return 0;
  }
  LOGGER_QUIET = args.quiet;
  MPI_Barrier(MPI_COMM_WORLD);
  double start = MPI_Wtime();
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myId);
  MPI_Get_processor_name(processorName, &nameLen);

  // Farm-like structure
  if (myId == 0) {
    // Emitter / Collector

    std::vector<Entity> entities;
    for (int i = 0; i < args.n_targets; i++) {
      Entity e(args.targets[i], args.recurse);
      if (!e) {
        LOG_W("Files", std::format("Entity `{}` does not exist", args.targets[i]));
        continue;
      }
      entities.emplace_back(std::move(e));
    }

    EC_work(entities, args.decompress, args.suff, args.split_size, args.n_threads);

  } else {
    // Worker
    W_work(myId);
  }

  double stop = MPI_Wtime();

  if (myId == 0) {
    setPrefix("");
    LOG_T("", std::format("{}", stop - start));
  }
  MPI_Finalize();
  return 0;
}