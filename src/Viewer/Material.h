/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include <functional>
#include "Render/RenderState.h"
#include "Render/Texture.h"
#include "Render/Uniform.h"
#include "Render/ShaderProgram.h"

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

  TextureUsage_SHADOWMAP,
};

enum MaterialType {
  Material_BaseColor,
  Material_Textured,
  Material_Skybox,
};

enum UniformBlockType {
  UniformBlock_Scene,
  UniformBlock_Model,
  UniformBlock_Material,
  UniformBlock_QuadFilter,
  UniformBlock_IBLPrefilter,
};

struct UniformsScene {
  glm::vec3 u_ambientColor;
  glm::vec3 u_cameraPosition;
  glm::vec3 u_pointLightPosition;
  glm::vec3 u_pointLightColor;
};

struct UniformsModel {
  glm::int32_t u_reverseZ;
  glm::mat4 u_modelMatrix;
  glm::mat4 u_modelViewProjectionMatrix;
  glm::mat3 u_inverseTransposeModelMatrix;
  glm::mat4 u_shadowMVPMatrix;
};

struct UniformsMaterial {
  glm::int32_t u_enableLight;
  glm::int32_t u_enableIBL;
  glm::int32_t u_enableShadow;

  float u_kSpecular;
  glm::vec4 u_baseColor;
};

struct UniformsQuadFilter {
  glm::vec2 u_screenSize;
};

struct UniformsIBLPrefilter {
  float u_srcResolution;
  float u_roughness;
};

struct TextureData {
  size_t width = 0;
  size_t height = 0;
  std::vector<std::shared_ptr<Buffer<RGBA>>> data;
  WrapMode wrapMode = Wrap_REPEAT;
};

class Material {
 public:
  static const char *shadingModelStr(ShadingModel model);
  static const char *textureUsageStr(TextureUsage usage);
  static const char *samplerDefine(TextureUsage usage);
  static const char *samplerName(TextureUsage usage);

  virtual MaterialType type() const = 0;

  virtual void reset() {
    shading = Shading_BaseColor;
    textureData.clear();
    resetRuntimeStates();
  }

  virtual void resetRuntimeStates() {
    renderState = RenderState();
    resetProgram();
    resetTextures();
  }

  void resetProgram() {
    shaderProgram = nullptr;
    shaderUniforms = nullptr;
    programDirty_ = true;
  }

  void resetTextures() {
    textures.clear();
    texturesDirty_ = true;
  }

  void createProgram(const std::function<void()> &func) {
    if (programDirty_) {
      func();
      programDirty_ = false;
    }
  }

  void createTextures(const std::function<void()> &func) {
    if (texturesDirty_) {
      func();
      texturesDirty_ = false;
    }
  }

 public:
  ShadingModel shading;
  AlphaMode alphaMode = Alpha_Opaque;
  bool doubleSided = false;
  std::unordered_map<int, TextureData> textureData;

  RenderState renderState;
  std::shared_ptr<ShaderProgram> shaderProgram;
  std::shared_ptr<ShaderUniforms> shaderUniforms;
  std::unordered_map<int, std::shared_ptr<Texture>> textures;

 private:
  bool programDirty_ = false;
  bool texturesDirty_ = false;
};

class BaseColorMaterial : public Material {
 public:
  MaterialType type() const override {
    return Material_BaseColor;
  }

 public:
  glm::vec4 baseColor;
};

class TexturedMaterial : public Material {
 public:
  MaterialType type() const override {
    return Material_Textured;
  }
};

class SkyboxMaterial : public Material {
 public:
  MaterialType type() const override {
    return Material_Skybox;
  }

  void resetRuntimeStates() override {
    Material::resetRuntimeStates();
    iblReady = false;
    iblError = false;
  }

 public:
  bool iblReady = false;
  bool iblError = false;
};

}
}
