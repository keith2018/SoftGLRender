/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "utils.h"


namespace SoftGL {

bool Utils::EndsWith(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

bool Utils::StartsWith(const std::string &str, const std::string &prefix) {
  return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

}