/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <fstream>
#include <sstream>
#include "logger.h"

namespace SoftGL {

class FileUtils {
 public:

  static std::string ReadAll(const std::string &path) {
    std::ifstream f(path.c_str());
    if (!f.is_open()) {
      LOGE("failed to open file: %s", path.c_str());
      return "";
    }
    std::stringstream buffer;
    buffer << f.rdbuf();
    return buffer.str();
  }

};

}
