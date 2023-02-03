/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include <functional>
#include "render/render_state.h"
#include "render/texture.h"
#include "render/uniform.h"
#include "render/shader_program.h"

namespace SoftGL {
namespace View {

enum AlphaMode {
  Alpha_Opaque,
  Alpha_Blend,
};

enum ShadingModel {
  Shading_BaseColor = 0,
  Shading_BlinnPhong,
  Shading_PBR,
  Shading_Skybox,
  Shading_IBL_Irradiance,
  Shading_IBL_Prefilter,
  Shading_FXAA,
};

enum TextureUsage {
  TextureUsage_NONE = 0,

  TextureUsage_ALBEDO,
  TextureUsage_NORMAL,
  TextureUsage_EMISSIVE,
  TextureUsage_AMBIENT_OCCLUSION,
  TextureUsage_METAL_ROUGHNESS,

  TextureUsage_CUBE,
  TextureUsage_EQUIRECTANGULAR,

  TextureUsage_IBL_IRRADIANCE,
  TextureUsage_IBL_PREFILTER,

  TextureUsage_QUAD_FILTER,
};

enum MaterialType {
  Material_BaseColor,
  Material_Textured,
  Material_Skybox,
};

struct UniformsScene {
  glm::int32_t u_enablePointLight;
  glm::int32_t u_enableIBL;

  glm::vec3 u_ambientColor;
  glm::vec3 u_cameraPosition;
  glm::vec3 u_pointLightPosition;
  glm::vec3 u_pointLightColor;
};

struct UniformsMVP {
  glm::mat4 u_modelMatrix;
  glm::mat4 u_modelViewProjectionMatrix;
  glm::mat3 u_inverseTransposeModelMatrix;
};

struct UniformsColor {
  glm::vec4 u_baseColor;
};

class Material {
 public:
  static const char *TextureUsageStr(TextureUsage usage);
  static const char *SamplerDefine(TextureUsage usage);
  static const char *SamplerName(TextureUsage usage);

  virtual MaterialType Type() const = 0;

  virtual void Reset() {
    shading = Shading_BaseColor;
    texture_data.clear();
    ResetRuntimeStates();
  }

  virtual void ResetRuntimeStates() {
    render_state = RenderState();
    ResetProgram();
    ResetTextures();
  }

  void ResetProgram() {
    shader_program = nullptr;
    shader_uniforms = nullptr;
    program_dirty_ = true;
  }

  void ResetTextures() {
    textures.clear();
    textures_dirty_ = true;
  }

  void CreateProgram(const std::function<void()> &func) {
    if (program_dirty_) {
      func();
      program_dirty_ = false;
    }
  }

  void CreateTextures(const std::function<void()> &func) {
    if (textures_dirty_) {
      func();
      textures_dirty_ = false;
    }
  }

 public:
  ShadingModel shading;
  std::unordered_map<int, std::vector<std::shared_ptr<BufferRGBA>>> texture_data;

  RenderState render_state;
  std::shared_ptr<ShaderProgram> shader_program;
  std::shared_ptr<ShaderUniforms> shader_uniforms;
  std::unordered_map<int, std::shared_ptr<Texture>> textures;

 private:
  bool program_dirty_ = false;
  bool textures_dirty_ = false;
};

class BaseColorMaterial : public Material {
 public:
  MaterialType Type() const override {
    return Material_BaseColor;
  }

 public:
  glm::vec4 base_color;
};

class TexturedMaterial : public Material {
 public:
  MaterialType Type() const override {
    return Material_Textured;
  }

 public:
  AlphaMode alpha_mode = Alpha_Opaque;
  bool double_sided = false;
};

class SkyboxMaterial : public Material {
 public:
  MaterialType Type() const override {
    return Material_Skybox;
  }

  void ResetRuntimeStates() override {
    Material::ResetRuntimeStates();
    ibl_ready = false;
  }

 public:
  bool ibl_ready = false;
};

}
}
