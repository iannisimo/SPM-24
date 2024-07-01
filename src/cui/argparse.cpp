#include "argparse.hpp"
#include <iostream>
#include <format>
#include "logger.hpp"
#include <getopt.h>
#include <string.h>

void config::argparse(int argc, char **argv) {
  pname = argv[0];
  int opt;
  int nt = 0;
  int ss = 0;
  int opt_index = 0;
  while ((opt = getopt_long(argc, argv, SHORT_OPTS, LONG_OPTIONS, &opt_index)) != -1) {
    switch (opt) {
      case 0:
        if (LONG_OPTIONS[opt_index].name[0] == 'm') {
          parallel = MPI;
          LOG_D("argparse", "Selected MPI");
        } else if (LONG_OPTIONS[opt_index].name[0] == 'f') {
          parallel = FASTFLOW;
          LOG_D("argparse", "Selected FastFlow");
        } else {
          parallel = SEQUENTIAL;
          LOG_D("argparse", "Selected sequential");
        }
        break;
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
        recurse = true;
        break;
      case 'q':
        quiet += 1;
        break;
      case 'S':
        // suff = optarg;
        suff = std::string(optarg);
        break;
      case 'j':
        nt = atoi(optarg);
        if (nt == 0) {
          LOG_E("argparse", "Option -j requires a number > 0 as argument");
          help = true;
        } else {
          n_threads = nt;
        }
        break;
      case 's':
        ss = atoi(optarg);
        if (ss == 0) {
          LOG_E("argparse", "Option -s requires a number > 0 as argumet");
          help = true;
        } else {
          split_size = ss;
        }
        break;
      case ':':
        LOG_E("args", std::format("Option -{} requires an argument", (char) optopt));
        help = true;
        break;
      case '?':
        LOG_W("args", std::format("Unknown option -{}", (char) optopt));
        help = true;
        break;
    }
  }
//  LOG_D("argparse", std::format("{}", optarg));
  targets = &argv[optind];
  n_targets = argc - optind;

  if (parallel == SEQUENTIAL) {
    n_threads = 1;
  } else {
    n_threads = n_threads < 2 ? 2 : n_threads;
  }
}