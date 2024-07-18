#ifndef _LOGGER
#define _LOGGER

#include <iostream>
#include <string>

int LOGGER_QUIET = 0;

#ifdef DEBUG
bool debug = true;
#else
bool debug = false;
#endif

std::string prefix = "";

void setPrefix(std::string pfx) {
  prefix = pfx;
}


void LOG_E(const std::string tag, const std::string msg) {
  if (LOGGER_QUIET > 2) return;
  if (tag[0] != 0) {
    std::cerr << prefix << tag << ": " << msg << std::endl;
  } else {
    std::cerr << prefix << msg << std::endl;
  }
}

void LOG_W(const std::string tag, const std::string msg) {
  if (LOGGER_QUIET > 1) return;
  if (tag[0] != 0) {
    std::cout << prefix << tag << ": " << msg << std::endl;
  } else {
    std::cout << prefix << msg << std::endl;
  }
}

void LOG_I(const std::string tag, const std::string msg) {
  if (LOGGER_QUIET > 0) return;
  if (tag[0] != 0) {
    std::cout << prefix << tag << ": " << msg << std::endl;
  } else {
    std::cout << prefix << msg << std::endl;
  }
}

void LOG_D(const std::string tag, const std::string msg) {
  if (!debug) return;
  if (LOGGER_QUIET > 3) return;
  if (tag[0] != 0) {
    std::cout << prefix << tag << ": " << msg << std::endl;
  } else {
    std::cout << prefix << msg << std::endl;
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