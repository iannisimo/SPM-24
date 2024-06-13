#ifndef _CUI_ARGPARSE
#define _CUI_ARGPARSE

#include <iostream>
#include <unistd.h>

const char* ARGS = ":hdkrqS:";

struct config {
  bool    help       = false;
  bool    decompress = false;
  bool    keep       = false;
  bool    recursive  = false;
  char*   suff       = NULL;
  char*   pname      = NULL;
  char**  targets    = NULL;
  int     n_targets  = 0;
  int     quiet      = 0;

  void argparse(int, char**);
};

#endif