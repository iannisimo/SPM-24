#include "cui/argparse.hpp"
#include "cui/usage.hpp"
#include "compressor/utils.hpp"
#include "compressor/sequential.hpp"
#include "compressor/fastflow.hpp"
#include "logger.hpp"
#include <format>
#include "miniz.h"
#include <string>

int main(int argc, char *argv[]) {

  config args;
  args.argparse(argc, argv);
  if (args.help) {
    usage(args.pname);
    return 0;
  }
  LOGGER_QUIET = args.quiet;

  std::vector<Entity> entities;
  for (int i = 0; i < args.n_targets; i++) {
    Entity e(args.targets[i], args.recurse);
    if (!e) {
      LOG_W("Files", std::format("Entity `{}` does not exist", args.targets[i]));
      continue;
    }
    entities.emplace_back(std::move(e));
  }

  switch (args.parallel) {
    case SEQUENTIAL:
      s_work(entities, args.decompress, args.suff, args.keep, args.split_size);
      break;
    case MPI:
      break;
    case FASTFLOW:
      f_work(entities, args.decompress, args.suff, args.keep, args.split_size, args.n_threads);
      break;
    default:
      LOG_E("", "In the name of god, how the hell did you end up here!");
      exit(1);
      break; 
  }

  return 0;
}