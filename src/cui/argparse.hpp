#ifndef CUI_ARGPARSE
#define CUI_ARGPARSE

#include <iostream>
#include <unistd.h>
#include <getopt.h>

const char* SHORT_OPTS = ":hdkrqS:j:s:";
const struct option LONG_OPTIONS[] = {};

struct config {
  bool          help       = false;
  bool          decompress = false;
  bool          keep       = false;
  bool          recurse    = false;
  std::string   suff       = ".spmzip";
  char*         pname      = NULL;
  char**        targets    = NULL;
  int           n_targets  = 0;
  int           quiet      = 0;
  int           n_threads  = 1;
  ulong         split_size = 1024UL;


  void argparse(int, char**);
};

#endif