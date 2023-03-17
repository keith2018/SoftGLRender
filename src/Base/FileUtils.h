/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <fstream>
#include "Logger.h"

namespace SoftGL {

class FileUtils {
 public:
  static std::vector<uint8_t> readBytes(const std::string &path) {
    std::vector<uint8_t> ret;
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      LOGE("failed to open file: %s", path.c_str());
      return ret;
    }

    size_t size = file.tellg();
    if (size <= 0) {
      LOGE("failed to read file, invalid size: %d", size);
      return ret;
    }

    ret.resize(size);

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char *>(ret.data()), (std::streamsize) size);

    return ret;
  }

  static std::string readText(const std::string &path) {
    auto data = readBytes(path);
    if (data.empty()) {
      return "";
    }

    return (char *) data.data();
  }

};

}
