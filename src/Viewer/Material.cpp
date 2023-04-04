/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "Material.h"

namespace SoftGL {
namespace View {

#define CASE_ENUM_STR(type) case type: return #type

const char *Material::shadingModelStr(ShadingModel model) {
  switch (model) {
    CASE_ENUM_STR(Shading_Unknown);
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

const char *Material::materialTexTypeStr(MaterialTexType usage) {
  switch (usage) {
    CASE_ENUM_STR(MaterialTexType_NONE);
    CASE_ENUM_STR(MaterialTexType_ALBEDO);
    CASE_ENUM_STR(MaterialTexType_NORMAL);
    CASE_ENUM_STR(MaterialTexType_EMISSIVE);
    CASE_ENUM_STR(MaterialTexType_AMBIENT_OCCLUSION);
    CASE_ENUM_STR(MaterialTexType_METAL_ROUGHNESS);
    CASE_ENUM_STR(MaterialTexType_CUBE);
    CASE_ENUM_STR(MaterialTexType_EQUIRECTANGULAR);
    CASE_ENUM_STR(MaterialTexType_IBL_IRRADIANCE);
    CASE_ENUM_STR(MaterialTexType_IBL_PREFILTER);
    CASE_ENUM_STR(MaterialTexType_QUAD_FILTER);
    CASE_ENUM_STR(MaterialTexType_SHADOWMAP);
    default:
      break;
  }
  return "";
}

const char *Material::samplerDefine(MaterialTexType usage) {
  switch (usage) {
    case MaterialTexType_ALBEDO:             return "ALBEDO_MAP";
    case MaterialTexType_NORMAL:             return "NORMAL_MAP";
    case MaterialTexType_EMISSIVE:           return "EMISSIVE_MAP";
    case MaterialTexType_AMBIENT_OCCLUSION:  return "AO_MAP";
    case MaterialTexType_METAL_ROUGHNESS:    return "METALROUGHNESS_MAP";
    case MaterialTexType_CUBE:               return "CUBE_MAP";
    case MaterialTexType_EQUIRECTANGULAR:    return "EQUIRECTANGULAR_MAP";
    case MaterialTexType_IBL_IRRADIANCE:
    case MaterialTexType_IBL_PREFILTER:      return "IBL_MAP";
    default:
      break;
  }

  return nullptr;
}

const char *Material::samplerName(MaterialTexType usage) {
  switch (usage) {
    case MaterialTexType_ALBEDO:             return "u_albedoMap";
    case MaterialTexType_NORMAL:             return "u_normalMap";
    case MaterialTexType_EMISSIVE:           return "u_emissiveMap";
    case MaterialTexType_AMBIENT_OCCLUSION:  return "u_aoMap";
    case MaterialTexType_METAL_ROUGHNESS:    return "u_metalRoughnessMap";
    case MaterialTexType_CUBE:               return "u_cubeMap";
    case MaterialTexType_EQUIRECTANGULAR:    return "u_equirectangularMap";
    case MaterialTexType_IBL_IRRADIANCE:     return "u_irradianceMap";
    case MaterialTexType_IBL_PREFILTER:      return "u_prefilterMap";
    case MaterialTexType_QUAD_FILTER:        return "u_screenTexture";
    case MaterialTexType_SHADOWMAP:          return "u_shadowMap";
    default:
      break;
  }

  return nullptr;
}

}
}
