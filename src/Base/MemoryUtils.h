/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <cmath>
#include <memory>
#include "Logger.h"

#define SOFTGL_ALIGNMENT 32

namespace SoftGL {

class MemoryUtils {
 public:

  static void *alignedMalloc(size_t size, size_t alignment = SOFTGL_ALIGNMENT) {
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
    void *alignedPtr = (void *) (addr - (addr % alignment));
    *((void **) alignedPtr - 1) = data;
    return alignedPtr;
  }

  static void alignedFree(void *ptr) {
    if (ptr) {
      free(((void **) ptr)[-1]);
    }
  }

  static size_t alignedSize(size_t size) {
    if (size == 0) {
      return 0;
    }
    return SOFTGL_ALIGNMENT * std::ceil((float) size / (float) SOFTGL_ALIGNMENT);
  }

  template<typename T>
  static std::shared_ptr<T> makeAlignedBuffer(size_t elemCnt) {
    if (elemCnt == 0) {
      return nullptr;
    }
    return std::shared_ptr<T>((T *) MemoryUtils::alignedMalloc(elemCnt * sizeof(T)),
                              [](const T *ptr) { MemoryUtils::alignedFree((void *) ptr); });
  }

  template<typename T>
  static std::shared_ptr<T> makeBuffer(size_t elemCnt, const uint8_t *data = nullptr) {
    if (elemCnt == 0) {
      return nullptr;
    }
    if (data != nullptr) {
      return std::shared_ptr<T>((T *) data, [](const T *ptr) {});
    } else {
      return std::shared_ptr<T>(new T[elemCnt], [](const T *ptr) { delete[] ptr; });
    }
  }
};

}
