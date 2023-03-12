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
  static std::shared_ptr<Buffer<RGBA>> readImageRGBA(const std::string &path);
  static void writeImage(char const *filename, int w, int h, int comp, const void *data, int strideInBytes,
                         bool flipY);

  static void convertFloatImage(RGBA *dst, float *src, int width, int height);
};

}
