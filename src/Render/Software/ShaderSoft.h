/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <functional>
#include "SamplerSoft.h"

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
  DerivativeContext dfCtx;
};

class ShaderSoft {
 public:
  virtual void shaderMain() = 0;

  virtual void bindDefines(void *ptr) = 0;
  virtual void bindBuiltin(void *ptr) = 0;
  virtual void bindShaderAttributes(void *ptr) = 0;
  virtual void bindShaderUniforms(void *ptr) = 0;
  virtual void bindShaderVaryings(void *ptr) = 0;

  virtual size_t getShaderUniformsSize() = 0;
  virtual size_t getShaderVaryingsSize() = 0;

  virtual std::vector<std::string> &getDefines() = 0;
  virtual std::vector<UniformDesc> &getUniformsDesc() = 0;

  virtual std::shared_ptr<ShaderSoft> clone() = 0;

 public:
  static inline glm::ivec2 textureSize(Sampler2DSoft<RGBA> *sampler, int lod) {
    auto &buffer = sampler->getTexture()->getImage().getBuffer(lod);
    return {buffer->width, buffer->height};
  }

  static inline glm::ivec2 textureSize(Sampler2DSoft<float> *sampler, int lod) {
    auto &buffer = sampler->getTexture()->getImage().getBuffer(lod);
    return {buffer->width, buffer->height};
  }

  static inline glm::vec4 texture(Sampler2DSoft<RGBA> *sampler, glm::vec2 coord) {
    glm::vec4 ret = sampler->texture2D(coord);
    return ret / 255.f;
  }

  static inline float texture(Sampler2DSoft<float> *sampler, glm::vec2 coord) {
    float ret = sampler->texture2D(coord);
    return ret;
  }

  static inline glm::vec4 texture(SamplerCubeSoft<RGBA> *sampler, glm::vec3 coord) {
    glm::vec4 ret = sampler->textureCube(coord);
    return ret / 255.f;
  }

  static inline glm::vec4 textureLod(Sampler2DSoft<RGBA> *sampler, glm::vec2 coord, float lod = 0.f) {
    glm::vec4 ret = sampler->texture2DLod(coord, lod);
    return ret / 255.f;
  }

  static inline glm::vec4 textureLod(SamplerCubeSoft<RGBA> *sampler, glm::vec3 coord, float lod = 0.f) {
    glm::vec4 ret = sampler->textureCubeLod(coord, lod);
    return ret / 255.f;
  }

  static inline glm::vec4 textureLodOffset(Sampler2DSoft<RGBA> *sampler,
                                           glm::vec2 coord,
                                           float lod,
                                           glm::ivec2 offset) {
    glm::vec4 ret = sampler->texture2DLodOffset(coord, lod, offset);
    return ret / 255.f;
  }

 public:
  ShaderBuiltin *gl = nullptr;
  std::function<float(BaseSampler<RGBA> *)> texLodFunc;

  float getSampler2DLod(BaseSampler<RGBA> *sampler) const {
    auto &dfCtx = gl->dfCtx;
    size_t dfOffset = getSamplerDerivativeOffset(sampler);

    auto *coord0 = (glm::vec2 *) (dfCtx.p0 + dfOffset);
    auto *coord1 = (glm::vec2 *) (dfCtx.p1 + dfOffset);
    auto *coord2 = (glm::vec2 *) (dfCtx.p2 + dfOffset);
    auto *coord3 = (glm::vec2 *) (dfCtx.p3 + dfOffset);

    glm::vec2 texSize = glm::vec2(sampler->width(), sampler->height());
    glm::vec2 dx = glm::vec2(*coord1 - *coord0);
    glm::vec2 dy = glm::vec2(*coord2 - *coord0);
    dx *= texSize;
    dy *= texSize;
    float d = glm::max(glm::dot(dx, dx), glm::dot(dy, dy));
    return glm::max(0.5f * glm::log2(d), 0.0f);
  }

  virtual void prepareExecMain() {
    texLodFunc = std::bind(&ShaderSoft::getSampler2DLod, this, std::placeholders::_1);
  }

  virtual size_t getSamplerDerivativeOffset(BaseSampler<RGBA> *sampler) const {
    return 0;
  }

  virtual void setupSamplerDerivative() {}

  int getUniformLocation(const std::string &name) {
    auto &desc = getUniformsDesc();
    for (int i = 0; i < desc.size(); i++) {
      if (desc[i].name == name) {
        return i;
      }
    }
    return -1;
  };

  int GetUniformOffset(int loc) {
    auto &desc = getUniformsDesc();
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
  void bindDefines(void *ptr) override {                \
    def = static_cast<ShaderDefines *>(ptr);            \
  }                                                     \
                                                        \
  void bindBuiltin(void *ptr) override {                \
    gl = static_cast<ShaderBuiltin *>(ptr);             \
  }                                                     \
                                                        \
  void bindShaderAttributes(void *ptr) override {       \
    a = static_cast<ShaderAttributes *>(ptr);           \
  }                                                     \
                                                        \
  void bindShaderUniforms(void *ptr) override {         \
    u = static_cast<ShaderUniforms *>(ptr);             \
  }                                                     \
                                                        \
  void bindShaderVaryings(void *ptr) override {         \
    v = static_cast<ShaderVaryings *>(ptr);             \
  }                                                     \
                                                        \
  size_t getShaderUniformsSize() override {             \
    return sizeof(ShaderUniforms);                      \
  }                                                     \
                                                        \
  size_t getShaderVaryingsSize() override {             \
    return sizeof(ShaderVaryings);                      \
  }

#define CREATE_SHADER_CLONE(T)                          \
  std::shared_ptr<ShaderSoft> clone() override {        \
    return std::make_shared<T>(*this);                  \
  }

}
