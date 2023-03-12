/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <fstream>
#include "logger.h"

namespace SoftGL {

class FileUtils {
 public:
  static std::string readAll(const std::string &path) {
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      LOGE("failed to open file: %s", path.c_str());
      return "";
    }

    size_t size = file.tellg();
    if (size <= 0) {
      LOGE("failed to read file, invalid size: %d", size);
      return "";
    }

    std::string content;
    content.resize(size);

    file.seekg(0, std::ios::beg);
    file.read(&content[0], (std::streamsize) size);

    return content;
  }

};

}
