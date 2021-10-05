/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Common.h"
#include "Texture.h"

namespace SoftGL {

#define CLONE_VERTEX_SHADER(T) \
std::shared_ptr<BaseVertexShader> clone() override { \
  auto *ret = new (T); \
  ret->uniforms = uniforms; \
  ret->attributes = attributes; \
  return std::shared_ptr<BaseVertexShader>(ret); \
} \


#define CLONE_FRAGMENT_SHADER(T) \
std::shared_ptr<BaseFragmentShader> clone() override { \
  auto *ret = new (T); \
  ret->uniforms = uniforms; \
  return std::shared_ptr<BaseFragmentShader>(ret); \
} \


#define ShaderAttributes Vertex

struct BaseShaderUniforms {
  glm::mat4 u_modelMatrix;
  glm::mat4 u_modelViewProjectionMatrix;
  glm::mat3 u_inverseTransposeModelMatrix;

  glm::vec3 u_cameraPosition;
  glm::vec3 u_ambientColor;

  bool u_showPointLight = false;
  glm::vec3 u_pointLightPosition;
  glm::vec3 u_pointLightColor;

  float u_alpha_cutoff = -1.f;
};

struct BaseShaderVaryings {
};

struct BaseShader {
  BaseShaderUniforms *uniforms = nullptr;
  virtual void shader_main() = 0;
};

struct BaseVertexShader : BaseShader {
  // inner output
  glm::vec4 gl_Position;

  ShaderAttributes *attributes;
  BaseShaderVaryings *varyings;

  void shader_main() override {
    auto *a = (ShaderAttributes *) attributes;
    auto *u = (BaseShaderUniforms *) uniforms;
    auto *v = (BaseShaderVaryings *) varyings;

    glm::vec4 position = glm::vec4(a->a_position, 1.0f);
    gl_Position = u->u_modelViewProjectionMatrix * position;
  }

  virtual std::shared_ptr<BaseVertexShader> clone() {
    auto *ret = new BaseVertexShader;
    ret->uniforms = uniforms;
    ret->attributes = attributes;
    return std::shared_ptr<BaseVertexShader>(ret);
  }
};

struct PixelQuadContext;
struct BaseFragmentShader : BaseShader {
  // inner input
  glm::vec4 gl_FragCoord;
  bool gl_FrontFacing;

  // inner output
  float gl_FragDepth;
  glm::vec4 gl_FragColor;
  bool discard = false;

  // varying
  BaseShaderVaryings *varyings;

  // quad context
  PixelQuadContext *quad_ctx;

  // texture lod
  std::function<float(Sampler &)> tex_lod_func;

  BaseFragmentShader() {
    tex_lod_func = std::bind(&BaseFragmentShader::GetTextureLod, this, std::placeholders::_1);
  }

  virtual float GetTextureLod(Sampler &sampler) {
    return 0.f;
  }

  void shader_main() override {
    gl_FragDepth = gl_FragCoord.z;
  };

  virtual std::shared_ptr<BaseFragmentShader> clone() {
    auto *ret = new BaseFragmentShader;
    ret->uniforms = uniforms;
    return std::shared_ptr<BaseFragmentShader>(ret);
  }
};

struct ShaderContext {
  size_t varyings_size;
  std::shared_ptr<BaseShaderUniforms> uniforms;
  std::shared_ptr<BaseVertexShader> vertex_shader;
  std::shared_ptr<BaseFragmentShader> fragment_shader;
};

}
