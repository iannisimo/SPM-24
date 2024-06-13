#include "cui/argparse.hpp"
#include "cui/usage.hpp"
#include "compressor/utils.hpp"
#include "miniz.h"
#include "logger.hpp"

int main(int argc, char *argv[]) {

  config args;
  args.argparse(argc, argv);
  if (args.help) {
    usage(args.pname);
    return 0;
  }
  LOGGER_QUIET = args.quiet;

  for (int i = 0; i < args.n_targets; i++) {
    Entity e(args.targets[i]);
    if (!e) {
      std::cout << args.targets[i] << " does not exits" << std::endl;
    }
    for (auto &&file : e.get_files()) {
      std::cout << file.get_abs_path() << std::endl;
    }
  }
  return 0;
}