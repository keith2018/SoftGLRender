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
    ResetTextures();
    ResetProgram();
  }

  virtual void ResetProgram() {
    shader_program = nullptr;
    program_dirty = true;
  }

  virtual void ResetTextures() {
    textures.clear();
    texture_data.clear();
    textures_dirty = true;
  }

  virtual void CreateProgram(const std::function<void()> &func) {
    if (program_dirty) {
      func();
      program_dirty = false;
    }
  }

  virtual void CreateTextures(const std::function<void()> &func) {
    if (textures_dirty) {
      func();
      textures_dirty = false;
    }
  }

 public:
  ShadingModel shading;
  RenderState render_state;
  std::shared_ptr<ShaderProgram> shader_program;

  std::unordered_map<int, std::shared_ptr<Texture>> textures;
  std::unordered_map<int, std::vector<std::shared_ptr<BufferRGBA>>> texture_data;

 private:
  bool program_dirty = false;
  bool textures_dirty = false;
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

  void Reset() override {
    Material::Reset();
    ibl_ready = false;
  }

 public:
  bool ibl_ready = false;
};

}
}
