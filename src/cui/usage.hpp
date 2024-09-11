#ifndef CUI_USAGE
#define CUI_USAGE

#include <iostream>

// Lamda function to print usage with program name
auto usage = [](const char* program_name) {
  std::cerr << "Usage: " << program_name << " [options] target1 [target2...]" << std::endl;
  std::cerr << "Options:" << std::endl;
  std::cerr << "  -h\t\tPrint this help message" << std::endl;
  std::cerr << "  -d\t\tDecompress the target(s)" << std::endl;
  std::cerr << "  -j NUM\tDefine the number of threads" << std::endl;
  std::cerr << "  -r\t\tRecursively process directories" << std::endl;
  std::cerr << "  -s DIM\tSpecify the max size of a zip part in bytes [default: inf]" << std::endl;
  std::cerr << "  -S SUFF\tSpecify a suffix to append to the target(s) [default: .spmcomp]" << std::endl;
};

#endif
