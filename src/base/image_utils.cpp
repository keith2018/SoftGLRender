/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include "image_utils.h"
#include "base/logger.h"

namespace SoftGL {

std::shared_ptr<Buffer<RGBA>> ImageUtils::ReadImageRGBA(const std::string &path) {
  int iw = 0, ih = 0, n = 0;
  unsigned char *data = stbi_load(path.c_str(), &iw, &ih, &n, STBI_default);
  if (data == nullptr) {
    LOGD("ImageUtils::ReadImage failed, path: %s", path.c_str());
    return nullptr;
  }
  auto buffer = Buffer<RGBA>::MakeDefault(iw, ih);

  // convert to rgba
  for (size_t y = 0; y < ih; y++) {
    for (size_t x = 0; x < iw; x++) {
      auto &to = *buffer->Get(x, y);
      size_t idx = x + y * iw;

      switch (n) {
        case STBI_grey: {
          to.r = data[idx];
          to.g = to.b = to.r;
          to.a = 255;
          break;
        }
        case STBI_grey_alpha: {
          to.r = data[idx * 2 + 0];
          to.g = to.b = to.r;
          to.a = data[idx * 2 + 1];
          break;
        }
        case STBI_rgb: {
          to.r = data[idx * 3 + 0];
          to.g = data[idx * 3 + 1];
          to.b = data[idx * 3 + 2];
          to.a = 255;
          break;
        }
        case STBI_rgb_alpha: {
          to.r = data[idx * 4 + 0];
          to.g = data[idx * 4 + 1];
          to.b = data[idx * 4 + 2];
          to.a = data[idx * 4 + 3];
          break;
        }
        default:
          break;
      }
    }
  }

  stbi_image_free(data);

  return buffer;
}

void ImageUtils::WriteImage(char const *filename,
                            int w,
                            int h,
                            int comp,
                            const void *data,
                            int stride_in_bytes,
                            bool flip_y) {
  stbi_flip_vertically_on_write(flip_y);
  stbi_write_png(filename, w, h, comp, data, stride_in_bytes);
}

void ImageUtils::ConvertFloatImage(RGBA *dst, float *src, int width, int height) {
  float *src_pixel = src;

  float depth_min = FLT_MAX;
  float depth_max = FLT_MIN;
  for (int i = 0; i < width * height; i++) {
    float depth = *src_pixel;
    depth_min = std::min(depth_min, depth);
    depth_max = std::max(depth_max, depth);
    src_pixel++;
  }

  src_pixel = src;
  RGBA *dst_pixel = dst;
  for (int i = 0; i < width * height; i++) {
    float depth = *src_pixel;
    depth = (depth - depth_min) / (depth_max - depth_min);
    dst_pixel->r = glm::clamp((int) (depth * 255.f), 0, 255);
    dst_pixel->g = dst_pixel->r;
    dst_pixel->b = dst_pixel->r;
    dst_pixel->a = 255;

    src_pixel++;
    dst_pixel++;
  }
}

}
