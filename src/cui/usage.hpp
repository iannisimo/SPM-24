#ifndef _USAGE
#define _USAGE

#include <iostream>

// Lamda function to print usage with program name
auto usage = [](const char* program_name) {
  std::cerr << "Usage: " << program_name << " [options] [target...]" << std::endl;
  std::cerr << "Options:" << std::endl;
  std::cerr << "  -h\t\tPrint this help message" << std::endl;
  std::cerr << "  -d\t\tDecompress the target(s)" << std::endl;
  std::cerr << "  -k\t\tKeep the target(s) after processing" << std::endl;
  std::cerr << "  -r\t\tRecursively process directories" << std::endl;
  std::cerr << "  -S suff\tSpecify a suffix to append to the target(s) [default: .spmcomp]" << std::endl;
  std::cerr << "Default:" << std::endl;
  std::cerr << "  Compress the target(s) deleting the original" << std::endl;
};

#endif