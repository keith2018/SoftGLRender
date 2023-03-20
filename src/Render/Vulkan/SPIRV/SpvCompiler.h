/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <vector>
#include <cstdint>

namespace SoftGL {

class SpvCompiler {
 public:
  static std::vector<uint32_t> compileVertexShader(const char *shaderSource);
  static std::vector<uint32_t> compileFragmentShader(const char *shaderSource);

};

}
