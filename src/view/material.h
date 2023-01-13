/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
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

enum TextureType {
  TextureType_NONE = 0,

  TextureType_ALBEDO,
  TextureType_NORMAL,
  TextureType_EMISSIVE,
  TextureType_AMBIENT_OCCLUSION,
  TextureType_METAL_ROUGHNESS,

  TextureType_IBL_IRRADIANCE,
  TextureType_IBL_PREFILTER,
};

class Material {
 public:
  static const char *TextureTypeStr(TextureType type);
  static const char *SamplerDefine(TextureType type);
  static const char *SamplerName(TextureType type);

  virtual void BindUniforms() {
    for (auto &group : uniform_groups) {
      shader_program->BindUniforms(group);
    }
  };

 public:
  ShadingModel shading;
  RenderState render_state;
  std::shared_ptr<ShaderProgram> shader_program;

  std::vector<std::vector<std::shared_ptr<Uniform>>> uniform_groups;
};

class BaseColorMaterial : public Material {
 public:
  glm::vec4 base_color;
};

class TexturedMaterial : public Material {
 public:
  std::unordered_map<int, std::shared_ptr<BufferRGBA>> texture_data;
  std::unordered_map<int, std::shared_ptr<Texture2D>> textures;

  AlphaMode alpha_mode = Alpha_Opaque;
  bool double_sided = false;
};

}
