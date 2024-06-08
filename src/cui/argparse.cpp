#include "argparse.hpp"
#include <iostream>

void config::argparse(int argc, char **argv) {
  pname = argv[0];
  int opt;
  while ((opt = getopt(argc, argv, ARGS)) != -1) {
    printf("opt = %c\n", opt);
    switch (opt) {
      case 'h':
        help = true;
        break;
      case 'd':
        decompress = true;
        break;
      case 'k':
        keep = true;
        break;
      case 'r':
        recursive = true;
        break;
      case 'S':
        suff = optarg;
        break;
      case ':':
        std::cerr << "Option -" << optopt << " requires an argument" << std::endl;
        break;
      case '?':
        std::cerr << "Unknown option -" << optopt << std::endl;
        break;
    }
  }
  targets = &argv[optind];
  n_targets = argc - optind;
}