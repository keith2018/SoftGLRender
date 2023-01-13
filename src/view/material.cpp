/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "material.h"

namespace SoftGL {

const char *Material::TextureTypeStr(TextureType type) {
#define TexType_STR(type) case type: return #type
  switch (type) {
    TexType_STR(TextureType_NONE);

    TexType_STR(TextureType_ALBEDO);
    TexType_STR(TextureType_NORMAL);
    TexType_STR(TextureType_EMISSIVE);
    TexType_STR(TextureType_AMBIENT_OCCLUSION);
    TexType_STR(TextureType_METAL_ROUGHNESS);

    TexType_STR(TextureType_IBL_IRRADIANCE);
    TexType_STR(TextureType_IBL_PREFILTER);
    default:break;
  }
  return "";
}

const char *Material::SamplerDefine(TextureType type) {
  switch (type) {
    case TextureType_ALBEDO: return "ALBEDO_MAP";
    case TextureType_NORMAL: return "NORMAL_MAP";
    case TextureType_EMISSIVE: return "EMISSIVE_MAP";
    case TextureType_AMBIENT_OCCLUSION: return "AO_MAP";
    default:break;
  }

  return nullptr;
}

const char *Material::SamplerName(TextureType type) {
  switch (type) {
    case TextureType_ALBEDO: return "u_albedoMap";
    case TextureType_NORMAL: return "u_normalMap";
    case TextureType_EMISSIVE: return "u_emissiveMap";
    case TextureType_AMBIENT_OCCLUSION: return "u_aoMap";
    case TextureType_METAL_ROUGHNESS: return "u_metalRoughnessMap";
    case TextureType_IBL_IRRADIANCE: return "u_irradianceMap";
    case TextureType_IBL_PREFILTER: return "u_prefilterMap";
    default:break;
  }

  return nullptr;
}

}
