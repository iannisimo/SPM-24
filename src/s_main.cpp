#include "cui/argparse.hpp"
#include "cui/usage.hpp"
#include "compressor/utils.hpp"
#include "compressor/sequential.hpp"
#include "logger.hpp"
#include <format>
#include "miniz.h"
#include <string>
#include <ctime> 

int main(int argc, char *argv[]) {

  std::chrono::time_point<std::chrono::system_clock> start, stop;
  start = std::chrono::system_clock::now();
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
      
      continue;
    }
    entities.emplace_back(std::move(e));
  }

  work(entities, args.decompress, args.suff, args.split_size);
  stop = std::chrono::system_clock::now();
  std::chrono::duration<double> delta = stop - start;
  

  return 0;
}