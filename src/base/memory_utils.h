/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <cmath>
#include <memory>
#include "logger.h"

#define SOFTGL_ALIGNMENT 32

namespace SoftGL {

class MemoryUtils {
 public:

  static void *AlignedMalloc(size_t size, size_t alignment = SOFTGL_ALIGNMENT) {
    if ((alignment & (alignment - 1)) != 0) {
      LOGE("failed to malloc, invalid alignment: %d", alignment);
      return nullptr;
    }

    size_t extra = alignment + sizeof(void *);
    void *data = malloc(size + extra);
    if (!data) {
      LOGE("failed to malloc with size: %d", size);
      return nullptr;
    }
    size_t addr = (size_t) data + extra;
    void *aligned_ptr = (void *) (addr - (addr % alignment));
    *((void **) aligned_ptr - 1) = data;
    return aligned_ptr;
  }

  static void AlignedFree(void *ptr) {
    if (ptr) {
      free(((void **) ptr)[-1]);
    }
  }

  static size_t AlignedSize(size_t size) {
    if (size == 0) {
      return 0;
    }
    return SOFTGL_ALIGNMENT * std::ceil((float) size / (float) SOFTGL_ALIGNMENT);
  }

  template<typename T>
  static std::shared_ptr<T> MakeAlignedBuffer(size_t size) {
    if (size == 0) {
      return nullptr;
    }
    return std::shared_ptr<T>((T *) MemoryUtils::AlignedMalloc(size),
                              [](const T *ptr) { MemoryUtils::AlignedFree((void *) ptr); });
  }

};

}
