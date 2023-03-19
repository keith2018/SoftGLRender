/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include <functional>
#include "Render/PipelineStates.h"
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
  Shading_Unknown = 0,
  Shading_BaseColor,
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

class MaterialObject {
 public:
  ShadingModel shadingModel = Shading_Unknown;
  std::shared_ptr<PipelineStates> pipelineStates;
  std::shared_ptr<ShaderProgram> shaderProgram;
  std::shared_ptr<ShaderResources> shaderResources;
};

class Material {
 public:
  static const char *shadingModelStr(ShadingModel model);
  static const char *textureUsageStr(TextureUsage usage);
  static const char *samplerDefine(TextureUsage usage);
  static const char *samplerName(TextureUsage usage);

 public:
  virtual void reset() {
    shadingModel = Shading_Unknown;
    doubleSided = false;
    alphaMode = Alpha_Opaque;

    baseColor = glm::vec4(1.f);
    pointSize = 1.f;
    lineWidth = 1.f;

    textureData.clear();

    shaderDefines.clear();
    textures.clear();
    materialObj = nullptr;
  }

  virtual void resetStates() {
    textures.clear();
    shaderDefines.clear();
    materialObj = nullptr;
  }

 public:
  ShadingModel shadingModel = Shading_Unknown;
  bool doubleSided = false;
  AlphaMode alphaMode = Alpha_Opaque;

  glm::vec4 baseColor = glm::vec4(1.f);
  float pointSize = 1.f;
  float lineWidth = 1.f;

  std::unordered_map<int, TextureData> textureData;

  std::set<std::string> shaderDefines;
  std::unordered_map<int, std::shared_ptr<Texture>> textures;
  std::shared_ptr<MaterialObject> materialObj = nullptr;
};

class SkyboxMaterial : public Material {
 public:
  void resetStates() override {
    Material::resetStates();
    iblReady = false;
  }

 public:
  bool iblReady = false;
};

}
}
