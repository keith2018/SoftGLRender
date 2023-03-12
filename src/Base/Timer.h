/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <chrono>
#include <string>

#include "Logger.h"

namespace SoftGL {

class Timer {
 public:
  void start();
  void stop();
  int64_t elapseMillis() const;

 private:
  std::chrono::time_point<std::chrono::steady_clock> start_;
  std::chrono::time_point<std::chrono::steady_clock> end_;
};

class ScopedTimer {
 public:
  explicit ScopedTimer(const char *str) : tag_(str) {
    timer_.start();
  }

  ~ScopedTimer() {
    timer_.stop();
    LOGD("%s: %lld ms\n", tag_.c_str(), timer_.elapseMillis());
  }

  explicit operator bool() {
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
