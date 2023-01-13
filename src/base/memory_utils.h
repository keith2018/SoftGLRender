/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <cstdlib>
#include "logger.h"

namespace SoftGL {

#define SOFTGL_ALIGNMENT_DEFAULT 32

class MemoryUtils {
 public:

  static void *AlignedMalloc(size_t size, size_t alignment = SOFTGL_ALIGNMENT_DEFAULT) {
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
};

}