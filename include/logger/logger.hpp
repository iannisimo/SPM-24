#ifndef _LOGGER
#define _LOGGER

#include <iostream>

int LOGGER_QUIET = 0;

#ifdef DEBUG
bool debug = true;
#else
bool debug = false;
#endif


void LOG_E(const std::string tag, const std::string msg) {
  if (LOGGER_QUIET > 2) return;
  if (tag[0] != 0) {
    std::cerr << tag << ": " << msg << std::endl;
  } else {
    std::cerr << msg << std::endl;
  }
}

void LOG_W(const std::string tag, const std::string msg) {
  if (LOGGER_QUIET > 1) return;
  if (tag[0] != 0) {
    std::cout << tag << ": " << msg << std::endl;
  } else {
    std::cout << msg << std::endl;
  }
}

void LOG_I(const std::string tag, const std::string msg) {
  if (LOGGER_QUIET > 0) return;
  if (tag[0] != 0) {
    std::cout << tag << ": " << msg << std::endl;
  } else {
    std::cout << msg << std::endl;
  }
}

void LOG_D(const std::string tag, const std::string msg) {
  if (LOGGER_QUIET > 3) return;
  if (!debug) return;
  if (tag[0] != 0) {
    std::cout << tag << ": " << msg << std::endl;
  } else {
    std::cout << msg << std::endl;
  }
}

void LOG_T(const std::string tag, const std::string msg) {
  if (LOGGER_QUIET > 4) return;
  if (tag[0] != 0) {
    std::cout << tag << ": " << msg << std::endl;
  } else {
    std::cout << msg << std::endl;
  }
}

#endif