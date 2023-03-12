/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "timer.h"

namespace SoftGL {

void Timer::start() {
  start_ = std::chrono::steady_clock::now();
}

void Timer::stop() {
  end_ = std::chrono::steady_clock::now();
}

int64_t Timer::elapseMillis() const {
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_ - start_);
  return duration.count();
}

}
