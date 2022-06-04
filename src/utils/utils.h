/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <string>

namespace SoftGL {

class Utils {
 public:

  static bool EndsWith(const std::string &str, const std::string &suffix);

  static bool StartsWith(const std::string &str, const std::string &prefix);
};

}
