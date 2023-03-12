/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "material.h"

namespace SoftGL {
namespace View {

#define CASE_ENUM_STR(type) case type: return #type

const char *Material::shadingModelStr(ShadingModel model) {
  switch (model) {
    CASE_ENUM_STR(Shading_BaseColor);
    CASE_ENUM_STR(Shading_BlinnPhong);
    CASE_ENUM_STR(Shading_PBR);
    CASE_ENUM_STR(Shading_Skybox);
    CASE_ENUM_STR(Shading_IBL_Irradiance);
    CASE_ENUM_STR(Shading_IBL_Prefilter);
    CASE_ENUM_STR(Shading_FXAA);
    default:
      break;
  }
  return "";
}

const char *Material::textureUsageStr(TextureUsage usage) {
  switch (usage) {
    CASE_ENUM_STR(TextureUsage_NONE);
    CASE_ENUM_STR(TextureUsage_ALBEDO);
    CASE_ENUM_STR(TextureUsage_NORMAL);
    CASE_ENUM_STR(TextureUsage_EMISSIVE);
    CASE_ENUM_STR(TextureUsage_AMBIENT_OCCLUSION);
    CASE_ENUM_STR(TextureUsage_METAL_ROUGHNESS);
    CASE_ENUM_STR(TextureUsage_CUBE);
    CASE_ENUM_STR(TextureUsage_EQUIRECTANGULAR);
    CASE_ENUM_STR(TextureUsage_IBL_IRRADIANCE);
    CASE_ENUM_STR(TextureUsage_IBL_PREFILTER);
    CASE_ENUM_STR(TextureUsage_QUAD_FILTER);
    CASE_ENUM_STR(TextureUsage_SHADOWMAP);
    default:
      break;
  }
  return "";
}

const char *Material::samplerDefine(TextureUsage usage) {
  switch (usage) {
    case TextureUsage_ALBEDO:             return "ALBEDO_MAP";
    case TextureUsage_NORMAL:             return "NORMAL_MAP";
    case TextureUsage_EMISSIVE:           return "EMISSIVE_MAP";
    case TextureUsage_AMBIENT_OCCLUSION:  return "AO_MAP";
    case TextureUsage_METAL_ROUGHNESS:    return "METALROUGHNESS_MAP";
    case TextureUsage_CUBE:               return "CUBE_MAP";
    case TextureUsage_EQUIRECTANGULAR:    return "EQUIRECTANGULAR_MAP";
    case TextureUsage_IBL_IRRADIANCE:
    case TextureUsage_IBL_PREFILTER:      return "IBL_MAP";
    default:
      break;
  }

  return nullptr;
}

const char *Material::samplerName(TextureUsage usage) {
  switch (usage) {
    case TextureUsage_ALBEDO:             return "u_albedoMap";
    case TextureUsage_NORMAL:             return "u_normalMap";
    case TextureUsage_EMISSIVE:           return "u_emissiveMap";
    case TextureUsage_AMBIENT_OCCLUSION:  return "u_aoMap";
    case TextureUsage_METAL_ROUGHNESS:    return "u_metalRoughnessMap";
    case TextureUsage_CUBE:               return "u_cubeMap";
    case TextureUsage_EQUIRECTANGULAR:    return "u_equirectangularMap";
    case TextureUsage_IBL_IRRADIANCE:     return "u_irradianceMap";
    case TextureUsage_IBL_PREFILTER:      return "u_prefilterMap";
    case TextureUsage_QUAD_FILTER:        return "u_screenTexture";
    case TextureUsage_SHADOWMAP:          return "u_shadowMap";
    default:
      break;
  }

  return nullptr;
}

}
}
