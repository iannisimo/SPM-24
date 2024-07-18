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
      case 'h':
        help = true;
        break;
      case 'd':
        decompress = true;
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
          
          help = true;
        } else {
          n_threads = nt;
        }
        break;
      case 's':
        ss = atoi(optarg);
        if (ss == 0) {
          
          help = true;
        } else {
          split_size = ss;
        }
        break;
      case ':':
        
        help = true;
        break;
      case '?':
        
        help = true;
        break;
    }
  }
//  
  targets = &argv[optind];
  n_targets = argc - optind;
}