/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <functional>
#include "sampler_soft.h"

namespace SoftGL {

class PixelQuadContext;
constexpr float PI = 3.14159265359;

class UniformDesc {
 public:
  UniformDesc(const char *name, int offset)
      : name(name), offset(offset) {};

 public:
  std::string name;
  int offset;
};

struct DerivativeContext {
  float *xa = nullptr;
  float *xb = nullptr;
  float *ya = nullptr;
  float *yb = nullptr;
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

  // derivative
  DerivativeContext df_ctx;
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

  virtual std::shared_ptr<ShaderSoft> clone() = 0;

 public:
  static inline glm::vec4 texture(Sampler2DSoft *sampler, glm::vec2 coord) {
    return sampler->Texture2D(coord);
  }

  static inline glm::vec4 texture(SamplerCubeSoft *sampler, glm::vec3 coord) {
    return sampler->TextureCube(coord);
  }

  static inline glm::vec4 textureLod(Sampler2DSoft *sampler, glm::vec2 coord, float lod = 0.f) {
    return sampler->Texture2DLod(coord, lod);
  }

  static inline glm::vec4 textureLod(SamplerCubeSoft *sampler, glm::vec3 coord, float lod = 0.f) {
    return sampler->TextureCubeLod(coord, lod);
  }

  static inline glm::vec4 textureLodOffset(Sampler2DSoft *sampler, glm::vec2 coord, float lod, glm::ivec2 offset) {
    return sampler->Texture2DLodOffset(coord, lod, offset);
  }

 public:
  ShaderBuiltin *gl = nullptr;
  std::function<float(BaseSampler<uint8_t> *)> tex_lod_func;

  float GetTexture2DLod(BaseSampler<uint8_t> *sampler) const {
    auto &df_ctx = gl->df_ctx;
    size_t df_offset = GetSamplerDerivativeOffset();

    auto *coord_xa = (glm::vec2 *) (df_ctx.xa + df_offset);
    auto *coord_xb = (glm::vec2 *) (df_ctx.xb + df_offset);
    auto *coord_ya = (glm::vec2 *) (df_ctx.ya + df_offset);
    auto *coord_yb = (glm::vec2 *) (df_ctx.yb + df_offset);

    glm::vec2 tex_size = glm::vec2(sampler->Width(), sampler->Height());
    glm::vec2 dx = glm::vec2(*coord_xa - *coord_xb) * tex_size;
    glm::vec2 dy = glm::vec2(*coord_ya - *coord_yb) * tex_size;
    float d = glm::max(glm::dot(dx, dx), glm::dot(dy, dy));
    return glm::max(0.5f * glm::log2(d), 0.0f);
  }

  virtual void PrepareExecMain() {
    tex_lod_func = std::bind(&ShaderSoft::GetTexture2DLod, this, std::placeholders::_1);
  }

  virtual size_t GetSamplerDerivativeOffset() const {
    return 0;
  }

  virtual void SetupSamplerDerivative() {}

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

#define CREATE_SHADER_CLONE(T)                          \
  std::shared_ptr<ShaderSoft> clone() override {        \
    return std::make_shared<T>(*this);                  \
  }

}
