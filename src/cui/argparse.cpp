#include "argparse.hpp"
#include <iostream>
#include <format>
#include "logger.hpp"

void config::argparse(int argc, char **argv) {
  pname = argv[0];
  int opt;
  while ((opt = getopt(argc, argv, ARGS)) != -1) {
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
      case 'q':
        quiet += 1;
        break;
      case 'S':
        suff = optarg;
        break;
      case ':':
        LOG_E("args", std::format("Option -{} requires an argument", (char) optopt));
        break;
      case '?':
        LOG_W("args", std::format("Unknown option -{}", (char) optopt));
        break;
    }
  }
  targets = &argv[optind];
  n_targets = argc - optind;
}