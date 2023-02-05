/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "sampler_soft.h"

namespace SoftGL {

class UniformDesc {
 public:
  UniformDesc(const char *name, int offset)
      : name(name), offset(offset) {};

 public:
  std::string name;
  int offset;
};

struct ShaderBuiltin {
  // vertex shader output
  glm::vec4 Position;

  // fragment shader input
  glm::vec4 FragCoord;
  bool FrontFacing;

  // fragment shader output
  float FragDepth;
  glm::vec4 FragColor;
  bool discard = false;
};

class ShaderSoft {
 public:
  virtual void ShaderMain() = 0;

  virtual void BindDefines(void *ptr) = 0;
  virtual void BindBuiltin(void *ptr) = 0;
  virtual void BindShaderAttributes(void *ptr) = 0;
  virtual void BindShaderUniforms(void *ptr) = 0;
  virtual void BindShaderVaryings(void *ptr) = 0;

  virtual size_t GetShaderUniformsSize() = 0;
  virtual size_t GetShaderVaryingsSize() = 0;

  virtual std::vector<std::string> &GetDefines() = 0;
  virtual std::vector<UniformDesc> &GetUniformsDesc() = 0;

 public:
  static inline glm::vec4 texture2D(Sampler2DSoft *sampler, glm::vec2 coord) {
    return sampler->Texture2D(coord);
  }

  static inline glm::vec4 textureCube(SamplerCubeSoft *sampler, glm::vec3 coord) {
    return sampler->TextureCube(coord);
  }

 public:
  int GetUniformLocation(const std::string &name) {
    auto &desc = GetUniformsDesc();
    for (int i = 0; i < desc.size(); i++) {
      if (desc[i].name == name) {
        return i;
      }
    }
    return -1;
  };

  int GetUniformOffset(int loc) {
    auto &desc = GetUniformsDesc();
    if (loc < 0 || loc > desc.size()) {
      return -1;
    }
    return desc[loc].offset;
  };
};

#define CREATE_SHADER_OVERRIDE                          \
  ShaderDefines *def = nullptr;                         \
  ShaderBuiltin *gl = nullptr;                          \
  ShaderAttributes *a = nullptr;                        \
  ShaderUniforms *u = nullptr;                          \
  ShaderVaryings *v = nullptr;                          \
                                                        \
  void BindDefines(void *ptr) override {                \
    def = static_cast<ShaderDefines *>(ptr);            \
  }                                                     \
                                                        \
  void BindBuiltin(void *ptr) override {                \
    gl = static_cast<ShaderBuiltin *>(ptr);             \
  }                                                     \
                                                        \
  void BindShaderAttributes(void *ptr) override {       \
    a = static_cast<ShaderAttributes *>(ptr);           \
  }                                                     \
                                                        \
  void BindShaderUniforms(void *ptr) override {         \
    u = static_cast<ShaderUniforms *>(ptr);             \
  }                                                     \
                                                        \
  void BindShaderVaryings(void *ptr) override {         \
    v = static_cast<ShaderVaryings *>(ptr);             \
  }                                                     \
                                                        \
  size_t GetShaderUniformsSize() override {             \
    return sizeof(ShaderUniforms);                      \
  }                                                     \
                                                        \
  size_t GetShaderVaryingsSize() override {             \
    return sizeof(ShaderVaryings);                      \
  }


}
