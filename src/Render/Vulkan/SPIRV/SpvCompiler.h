/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>

namespace SoftGL {

enum ShaderUniformType {
  UniformType_Unknown,
  UniformType_Sampler,
  UniformType_Block,
};

struct ShaderUniformDesc {
  std::string name;
  ShaderUniformType type;
  int location;
  int binding;
  int set;
};

struct ShaderCompilerResult {
  std::vector<uint32_t> spvCodes;
  std::unordered_map<std::string, ShaderUniformDesc> uniformsDesc;
};

class SpvCompiler {
 public:
  static ShaderCompilerResult compileVertexShader(const char *shaderSource);
  static ShaderCompilerResult compileFragmentShader(const char *shaderSource);

};

}
