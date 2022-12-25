/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/common.h"

namespace SoftGL {

class MemoryUtils {
 public:

  static void *AlignedMalloc(size_t size, size_t alignment = SOFTGL_ALIGNMENT) {
    assert((alignment & (alignment - 1)) == 0);

    size_t extra = alignment + sizeof(void *);
    void *data = malloc(size + extra);
    if (!data) {
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