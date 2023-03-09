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
  float *p0 = nullptr;
  float *p1 = nullptr;
  float *p2 = nullptr;
  float *p3 = nullptr;
};

struct ShaderBuiltin {
  // vertex shader output
  glm::vec4 Position;

  // fragment shader input
  glm::vec4 FragCoord;
  bool FrontFacing;

  // fragment shader output
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
  static inline glm::ivec2 textureSize(Sampler2DSoft<RGBA> *sampler, int lod) {
    auto &buffer = sampler->GetTexture()->GetImage().GetBuffer(lod);
    return {buffer->width, buffer->height};
  }

  static inline glm::ivec2 textureSize(Sampler2DSoft<float> *sampler, int lod) {
    auto &buffer = sampler->GetTexture()->GetImage().GetBuffer(lod);
    return {buffer->width, buffer->height};
  }

  static inline glm::vec4 texture(Sampler2DSoft<RGBA> *sampler, glm::vec2 coord) {
    glm::vec4 ret = sampler->Texture2D(coord);
    return ret / 255.f;
  }

  static inline float texture(Sampler2DSoft<float> *sampler, glm::vec2 coord) {
    float ret = sampler->Texture2D(coord);
    return ret;
  }

  static inline glm::vec4 texture(SamplerCubeSoft<RGBA> *sampler, glm::vec3 coord) {
    glm::vec4 ret = sampler->TextureCube(coord);
    return ret / 255.f;
  }

  static inline glm::vec4 textureLod(Sampler2DSoft<RGBA> *sampler, glm::vec2 coord, float lod = 0.f) {
    glm::vec4 ret = sampler->Texture2DLod(coord, lod);
    return ret / 255.f;
  }

  static inline glm::vec4 textureLod(SamplerCubeSoft<RGBA> *sampler, glm::vec3 coord, float lod = 0.f) {
    glm::vec4 ret = sampler->TextureCubeLod(coord, lod);
    return ret / 255.f;
  }

  static inline glm::vec4 textureLodOffset(Sampler2DSoft<RGBA> *sampler,
                                           glm::vec2 coord,
                                           float lod,
                                           glm::ivec2 offset) {
    glm::vec4 ret = sampler->Texture2DLodOffset(coord, lod, offset);
    return ret / 255.f;
  }

 public:
  ShaderBuiltin *gl = nullptr;
  std::function<float(BaseSampler<RGBA> *)> tex_lod_func;

  float GetSampler2DLod(BaseSampler<RGBA> *sampler) const {
    auto &df_ctx = gl->df_ctx;
    size_t df_offset = GetSamplerDerivativeOffset(sampler);

    auto *coord_0 = (glm::vec2 *) (df_ctx.p0 + df_offset);
    auto *coord_1 = (glm::vec2 *) (df_ctx.p1 + df_offset);
    auto *coord_2 = (glm::vec2 *) (df_ctx.p2 + df_offset);
    auto *coord_3 = (glm::vec2 *) (df_ctx.p3 + df_offset);

    glm::vec2 tex_size = glm::vec2(sampler->Width(), sampler->Height());
    glm::vec2 dx = glm::vec2(*coord_1 - *coord_0);
    glm::vec2 dy = glm::vec2(*coord_2 - *coord_0);
    dx *= tex_size;
    dy *= tex_size;
    float d = glm::max(glm::dot(dx, dx), glm::dot(dy, dy));
    return glm::max(0.5f * glm::log2(d), 0.0f);
  }

  virtual void PrepareExecMain() {
    tex_lod_func = std::bind(&ShaderSoft::GetSampler2DLod, this, std::placeholders::_1);
  }

  virtual size_t GetSamplerDerivativeOffset(BaseSampler<RGBA> *sampler) const {
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
