/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "utils.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace SoftGL {

std::unordered_map<std::string, std::shared_ptr<Buffer<glm::u8vec4>>> Utils::texture_cache_;

bool Utils::LoadTextureFile(SoftGL::Texture &tex, const char *path) {
  if (texture_cache_.find(path) != texture_cache_.end()) {
    tex.buffer = texture_cache_[path];
    return true;
  }

  std::cout << "load texture, path: " << path << std::endl;

  int iw = 0, ih = 0, n = 0;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data = stbi_load(path, &iw, &ih, &n, STBI_default);
  if (data == nullptr) {
    return false;
  }
  tex.buffer = Texture::CreateBufferDefault();
  tex.buffer->Create(iw, ih);
  texture_cache_[path] = tex.buffer;
  auto &buffer = tex.buffer;

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
        default:break;
      }
    }
  }

  stbi_image_free(data);
  return true;
}

}