#include "cui/argparse.hpp"
#include "cui/usage.hpp"

int main(int argc, char *argv[]) {
  config args;
  args.argparse(argc, argv);
  if (args.help) {
    usage(args.pname);
    return 0;
  }
  for (int i = 0; i < args.n_targets; i++) {
    std::cout << args.targets[i] << std::endl;
  }
  return 0;
}