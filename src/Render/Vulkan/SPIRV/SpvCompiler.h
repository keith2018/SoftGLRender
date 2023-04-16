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

enum ShaderStage {
  ShaderStage_Vertex,
  ShaderStage_Fragment,
};

enum ShaderUniformType {
  UniformType_Unknown = 0,
  UniformType_Sampler = 1,
  UniformType_Block = 2,
};

struct ShaderUniformDesc {
  std::string name;
  ShaderUniformType type = UniformType_Unknown;
  int location = 0;
  int binding = 0;
  int set = 0;
};

struct ShaderCompilerResult {
  std::vector<uint32_t> spvCodes;
  std::unordered_map<std::string, ShaderUniformDesc> uniformsDesc;
};

class SpvCompiler {
 public:
  static ShaderCompilerResult compileVertexShader(const char *shaderSource);
  static ShaderCompilerResult compileFragmentShader(const char *shaderSource);

 private:
  static ShaderCompilerResult compileShader(const char *shaderSource, ShaderStage stage);
};

}
