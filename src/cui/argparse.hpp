#ifndef _ARGPARSE
#define _ARGPARSE

#include <iostream>
#include <unistd.h>

const char* ARGS = ":hdkrS:";

struct config {
  bool    help       = false;
  bool    decompress = false;
  bool    keep       = false;
  bool    recursive  = false;
  char*   suff       = NULL;
  char*   pname      = NULL;
  char**  targets    = NULL;
  int     n_targets  = 0;

  void argparse(int, char**);
};

#endif