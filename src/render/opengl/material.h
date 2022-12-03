/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include <glad/glad.h>
#include "base/common.h"
#include "base/logger.h"
#include "base/model.h"
#include "shader_utils.h"
#include "opengl_utils.h"
#include "shader/shader_glsl.h"

namespace SoftGL {

class UniformsBlockGLSL {
 public:
  bool Empty() const {
    return 0 == ubo_;
  }

  virtual ~UniformsBlockGLSL() {
    glDeleteBuffers(1, &ubo_);
  }

  virtual void Init() = 0;
  virtual void UpdateData() = 0;

  void BindProgram(GLuint program, GLint blockIndex, GLuint binding) const {
    glUniformBlockBinding(program, blockIndex, binding);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, ubo_);
  }

 protected:
  void Create(GLint blockSize) {
    glGenBuffers(1, &ubo_);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, blockSize, nullptr, GL_STATIC_DRAW);
  }

 public:
  GLuint ubo_ = 0;
};

class UniformsMVP : public UniformsBlockGLSL {
 public:
  void Init() override {
    Create(sizeof(data));
  }

  void UpdateData() override {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
  }

 public:
  struct {
    glm::mat4 u_modelMatrix;
    glm::mat4 u_modelViewProjectionMatrix;
    glm::mat3 u_inverseTransposeModelMatrix;
  } data;
};

class UniformsColor : public UniformsBlockGLSL {
 public:
  void Init() override {
    Create(sizeof(data));
  }

  void UpdateData() override {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
  }

 public:
  struct {
    glm::vec4 u_baseColor;
  } data;
};

class UniformsLight : public UniformsBlockGLSL {
 public:
  void Init() override {
    Create(sizeof(data));
  }

  void UpdateData() override {
    glBufferData(GL_UNIFORM_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
  }

 public:
  struct {
    glm::vec3 u_ambientColor;

    glm::vec3 u_cameraPosition;
    glm::vec3 u_pointLightPosition;
    glm::vec3 u_pointLightColor;
  } data;

  bool show_light = false;
};

class UniformsAlphaMask : public UniformsBlockGLSL {
 public:
  void Init() override {
    Create(sizeof(float));
  }

  void UpdateData() override {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float), &data.u_alpha_cutoff);
  }

 public:
  struct {
    float u_alpha_cutoff;
  } data;
};

class RendererUniforms {
 public:
  void Init() {
    if (uniforms_mvp.Empty()) {
      uniforms_mvp.Init();
    }
    if (uniforms_color.Empty()) {
      uniforms_color.Init();
    }
    if (uniforms_light.Empty()) {
      uniforms_light.Init();
    }
    if (uniforms_alpha_mask.Empty()) {
      uniforms_alpha_mask.Init();
    }
  }

 public:
  UniformsMVP uniforms_mvp;
  UniformsColor uniforms_color;
  UniformsLight uniforms_light;
  UniformsAlphaMask uniforms_alpha_mask;
};

class TextureGLSL {
 public:
  void Create(Texture &tex);
  bool Empty() const;
  void BindProgram(GLint samplerLoc, GLuint binding) const;
  virtual ~TextureGLSL();

 private:
  GLuint texId_ = 0;
};

enum MaterialType {
  MaterialType_BaseColor = 0,
  MaterialType_BaseTexture,
  MaterialType_BlinnPhong,
  MaterialType_PbrBase,
  MaterialType_PbrLight,
};

using TextureMap = std::unordered_map<TextureType, std::shared_ptr<TextureGLSL>>;

class BaseMaterial {
 public:
  virtual MaterialType Type() = 0;
  virtual void Init() = 0;
  virtual void Use(RendererUniforms &uniforms, TextureMap &textures) = 0;
 public:
  ProgramGLSL program;

  GLint uniforms_loc_mvp = 0;
  GLint uniforms_loc_color = 0;
  GLint uniforms_loc_light = 0;
  GLint uniforms_loc_alpha_mask = 0;

  std::unordered_map<TextureType, GLint> sampler_loc;
};

class MaterialBaseColor : public BaseMaterial {
 public:
  inline MaterialType Type() override { return MaterialType_BaseColor; };
  void Init() override;
  void Use(RendererUniforms &uniforms, TextureMap &textures) override;
};

class MaterialBaseTexture : public BaseMaterial {
 public:
  inline MaterialType Type() override { return MaterialType_BaseTexture; };
  void Init() override;
  void Use(RendererUniforms &uniforms, TextureMap &textures) override;

  void SetEnableNormalMap(bool enabled) {
    enable_normal_map_ = enabled;
  }
  void SetEnableEmissiveMap(bool enabled) {
    enable_emissive_map_ = enabled;
  }
  void SetEnableAoMap(bool enabled) {
    enable_ao_map_ = enabled;
  }
  void SetEnableAlphaDiscard(bool enabled) {
    enable_alpha_discard_ = enabled;
  }

 private:
  bool enable_normal_map_ = false;
  bool enable_emissive_map_ = false;
  bool enable_ao_map_ = false;
  bool enable_alpha_discard_ = false;
};

class MaterialBlinnPhong : public MaterialBaseTexture {
 public:
  inline MaterialType Type() override { return MaterialType_BlinnPhong; };
  void Init() override;
};

}
