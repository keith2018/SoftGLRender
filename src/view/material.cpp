/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "material.h"

namespace SoftGL {

const char *Material::TextureUsageStr(TextureUsage usage) {
#define TexType_STR(type) case type: return #type
  switch (usage) {
    TexType_STR(TextureUsage_NONE);
    TexType_STR(TextureUsage_ALBEDO);
    TexType_STR(TextureUsage_NORMAL);
    TexType_STR(TextureUsage_EMISSIVE);
    TexType_STR(TextureUsage_AMBIENT_OCCLUSION);
    TexType_STR(TextureUsage_METAL_ROUGHNESS);
    TexType_STR(TextureUsage_CUBE);
    TexType_STR(TextureUsage_EQUIRECTANGULAR);
    TexType_STR(TextureUsage_IBL_IRRADIANCE);
    TexType_STR(TextureUsage_IBL_PREFILTER);
    default:
      break;
  }
  return "";
}

const char *Material::SamplerDefine(TextureUsage usage) {
  switch (usage) {
    case TextureUsage_ALBEDO:
      return "ALBEDO_MAP";
    case TextureUsage_NORMAL:
      return "NORMAL_MAP";
    case TextureUsage_EMISSIVE:
      return "EMISSIVE_MAP";
    case TextureUsage_AMBIENT_OCCLUSION:
      return "AO_MAP";
    case TextureUsage_METAL_ROUGHNESS:
      return "METALROUGHNESS_MAP";
    case TextureUsage_CUBE:
      return "CUBE_MAP";
    case TextureUsage_EQUIRECTANGULAR:
      return "EQUIRECTANGULAR_MAP";
    case TextureUsage_IBL_IRRADIANCE:
    case TextureUsage_IBL_PREFILTER:
      return "IBL_MAP";
    default:
      break;
  }

  return nullptr;
}

const char *Material::SamplerName(TextureUsage usage) {
  switch (usage) {
    case TextureUsage_ALBEDO:
      return "u_albedoMap";
    case TextureUsage_NORMAL:
      return "u_normalMap";
    case TextureUsage_EMISSIVE:
      return "u_emissiveMap";
    case TextureUsage_AMBIENT_OCCLUSION:
      return "u_aoMap";
    case TextureUsage_METAL_ROUGHNESS:
      return "u_metalRoughnessMap";
    case TextureUsage_CUBE:
      return "u_cubeMap";
    case TextureUsage_EQUIRECTANGULAR:
      return "u_equirectangularMap";
    case TextureUsage_IBL_IRRADIANCE:
      return "u_irradianceMap";
    case TextureUsage_IBL_PREFILTER:
      return "u_prefilterMap";
    default:
      break;
  }

  return nullptr;
}

}
