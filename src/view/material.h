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
};

enum SkyboxType {
  Skybox_Cube,
  Skybox_Equirectangular,
};

enum MaterialType {
  Material_BaseColor,
  Material_Textured,
  Material_Skybox,
};

class Material {
 public:
  static const char *TextureUsageStr(TextureUsage usage);
  static const char *SamplerDefine(TextureUsage usage);
  static const char *SamplerName(TextureUsage usage);

  virtual MaterialType Type() const = 0;

  virtual void BindUniforms() {
    for (auto &group : uniform_groups) {
      if (!group.empty()) {
        shader_program->BindUniforms(group);
      }
    }
  };

  virtual void SetDirty() {
    dirty = true;
    shader_program = nullptr;
    uniform_groups.clear();
    texture_data.clear();
  }

  virtual void InitIfDirty(const std::function<void()> &func) {
    if (dirty) {
      dirty = false;
      func();
    }
  }

 public:
  ShadingModel shading;
  RenderState render_state;
  std::shared_ptr<ShaderProgram> shader_program;
  std::vector<std::vector<std::shared_ptr<Uniform>>> uniform_groups;

  std::unordered_map<int, std::vector<std::shared_ptr<BufferRGBA>>> texture_data;

 private:
  bool dirty = false;
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

class SkyboxMaterial : public TexturedMaterial {
 public:
  MaterialType Type() const override {
    return Material_Skybox;
  }

 public:
  SkyboxType skybox_type = Skybox_Cube;
};

}
