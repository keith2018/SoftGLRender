/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/buffer.h"

namespace SoftGL {

class ImageUtils {
 public:
  static std::shared_ptr<Buffer<RGBA>> ReadImageRGBA(const std::string &path);

  static void WriteImage(char const *filename,
                         int w,
                         int h,
                         int comp,
                         const void *data,
                         int stride_in_bytes,
                         bool flip_y);

  static void ConvertFloatImage(RGBA *dst, float *src, int width, int height);
};

}
