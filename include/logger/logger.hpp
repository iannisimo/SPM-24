#ifndef _LOGGER
#define _LOGGER

#include <iostream>

bool quiet = false;

#ifdef DEBUG
bool debug = true;
#else
bool debug = false;
#endif


void LOG_E(const std::string tag, const std::string msg) {
  perror(tag.c_str());
  std::cerr << msg << std::endl;
}

void LOG_I(const std::string tag, const std::string msg) {
  if (tag[0] != 0) {
    std::cout << tag << ": " << msg << std::endl;
  } else {
    std::cout << msg << std::endl;
  }
}

void LOG_W(const std::string tag, const std::string msg) {
  if (quiet) return;
  if (tag[0] != 0) {
    std::cout << tag << ": " << msg << std::endl;
  } else {
    std::cout << msg << std::endl;
  }
}

void LOG_D(const std::string tag, const std::string msg) {
  if (!debug) return;
  if (tag[0] != 0) {
    std::cout << tag << ": " << msg << std::endl;
  } else {
    std::cout << msg << std::endl;
  }
}

#endif