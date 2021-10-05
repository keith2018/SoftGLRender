/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "Timer.h"

namespace SoftGL {

void Timer::Start() {
  start_ = std::chrono::steady_clock::now();
}

void Timer::Stop() {
  end_ = std::chrono::steady_clock::now();
}

int64_t Timer::ElapseMillis() const {
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_ - start_);
  return duration.count();
}

}
