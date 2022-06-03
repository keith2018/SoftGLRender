/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <string>
#include <unordered_map>
#include "renderer/texture.h"

namespace SoftGL {

class Utils {
 public:

  static bool EndsWith(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
  }

  static bool StartsWith(const std::string &str, const std::string &prefix) {
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
  }

  static bool LoadTextureFile(Texture &tex, const char *path);

 private:
  static std::unordered_map<std::string, std::shared_ptr<Buffer<glm::u8vec4>>> texture_cache_;
};

}
