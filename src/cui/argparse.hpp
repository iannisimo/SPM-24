#ifndef CUI_ARGPARSE
#define CUI_ARGPARSE

#include <iostream>
#include <unistd.h>
#include <getopt.h>

const char* SHORT_OPTS = ":hdkrqS:j:s:";
const struct option LONG_OPTIONS[] = {
  {"mpi", no_argument, 0, 0},
  {"fastflow", no_argument, 0, 0},
  {"sequential", no_argument, 0, 0}
};

typedef enum _parallel {
  SEQUENTIAL,
  MPI,
  FASTFLOW
} parallel_impl;

struct config {
  bool          help       = false;
  bool          decompress = false;
  bool          keep       = false;
  bool          recursive  = false;
  char*         suff       = NULL;
  char*         pname      = NULL;
  char**        targets    = NULL;
  int           n_targets  = 0;
  int           quiet      = 0;
  int           n_threads  = 1;
  int           split_size = 1024;
  parallel_impl parallel   = SEQUENTIAL;


  void argparse(int, char**);
};

#endif