/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <chrono>
#include <string>

#include "logger.h"

namespace SoftGL {

class Timer {
 public:
  Timer() {}

  ~Timer() {}

  void Start();

  void Stop();

  int64_t ElapseMillis() const;

 private:
  std::chrono::time_point<std::chrono::steady_clock> start_;
  std::chrono::time_point<std::chrono::steady_clock> end_;
};

class ScopedTimer {
 public:

  ScopedTimer(const char *str)
      : tag_(str) {
    timer_.Start();
  }

  ~ScopedTimer() {
    timer_.Stop();
    LOGD("%s: %lld ms\n", tag_.c_str(), timer_.ElapseMillis());
  }

  operator bool() {
    return true;
  }

 private:
  Timer timer_;
  std::string tag_;
};

#ifndef NDEBUG
#   define TIMED(X) if(SoftGL::ScopedTimer _ScopedTimer = (X))
#else
#   define TIMED(X)
#endif

}
