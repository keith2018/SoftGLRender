/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/soft/shader_program_soft.h"

namespace SoftGL {
namespace ShaderPbrIBL {

struct ShaderDefines {
  uint8_t ALBEDO_MAP;
  uint8_t NORMAL_MAP;
  uint8_t EMISSIVE_MAP;
  uint8_t AO_MAP;
  uint8_t METALROUGHNESS_MAP;
};

struct ShaderAttributes {
  glm::vec3 a_position;
  glm::vec2 a_texCoord;
  glm::vec3 a_normal;
  glm::vec3 a_tangent;
};

struct ShaderUniforms {
  // UniformsModel
  glm::int32_t u_reverseZ;
  glm::mat4 u_modelMatrix;
  glm::mat4 u_modelViewProjectionMatrix;
  glm::mat3 u_inverseTransposeModelMatrix;
  glm::mat4 u_shadowMVPMatrix;

  // UniformsScene
  glm::vec3 u_ambientColor;
  glm::vec3 u_cameraPosition;
  glm::vec3 u_pointLightPosition;
  glm::vec3 u_pointLightColor;

  // UniformsMaterial
  glm::int32_t u_enableLight;
  glm::int32_t u_enableIBL;
  glm::int32_t u_enableShadow;
  float u_kSpecular;
  glm::vec4 u_baseColor;

  // Samplers
  Sampler2DSoft<RGBA> *u_albedoMap;
  Sampler2DSoft<RGBA> *u_normalMap;
  Sampler2DSoft<RGBA> *u_emissiveMap;
  Sampler2DSoft<RGBA> *u_aoMap;

  Sampler2DSoft<RGBA> *u_metalRoughnessMap;

  SamplerCubeSoft<RGBA> *u_irradianceMap;
  SamplerCubeSoft<RGBA> *u_prefilterMap;
};

struct ShaderVaryings {
  glm::vec2 v_texCoord;
  glm::vec3 v_normalVector;
  glm::vec3 v_worldPos;
  glm::vec3 v_cameraDirection;
  glm::vec3 v_lightDirection;

  glm::vec3 v_normal;
  glm::vec3 v_tangent;
};

class ShaderPbrIBL : public ShaderSoft {
 public:
  CREATE_SHADER_OVERRIDE

  std::vector<std::string> &getDefines() override {
    static std::vector<std::string> defines = {
        "ALBEDO_MAP",
        "NORMAL_MAP",
        "EMISSIVE_MAP",
        "AO_MAP",
        "METALROUGHNESS_MAP",
    };
    return defines;
  }

  std::vector<UniformDesc> &getUniformsDesc() override {
    static std::vector<UniformDesc> desc = {
        {"UniformsModel", offsetof(ShaderUniforms, u_reverseZ)},
        {"UniformsScene", offsetof(ShaderUniforms, u_ambientColor)},
        {"UniformsMaterial", offsetof(ShaderUniforms, u_enableLight)},
        {"u_albedoMap", offsetof(ShaderUniforms, u_albedoMap)},
        {"u_normalMap", offsetof(ShaderUniforms, u_normalMap)},
        {"u_emissiveMap", offsetof(ShaderUniforms, u_emissiveMap)},
        {"u_aoMap", offsetof(ShaderUniforms, u_aoMap)},
        {"u_metalRoughnessMap", offsetof(ShaderUniforms, u_metalRoughnessMap)},
        {"u_irradianceMap", offsetof(ShaderUniforms, u_irradianceMap)},
        {"u_prefilterMap", offsetof(ShaderUniforms, u_prefilterMap)},
    };
    return desc;
  };
};

class VS : public ShaderPbrIBL {
 public:
  CREATE_SHADER_CLONE(VS)

  void shaderMain() override {
    glm::vec4 position = glm::vec4(a->a_position, 1.0);
    gl->Position = u->u_modelViewProjectionMatrix * position;
    v->v_texCoord = a->a_texCoord;

    // world space
    v->v_worldPos = glm::vec3(u->u_modelMatrix * position);
    v->v_normalVector = glm::mat3(u->u_modelMatrix) * a->a_normal;
    v->v_lightDirection = u->u_pointLightPosition - v->v_worldPos;
    v->v_cameraDirection = u->u_cameraPosition - v->v_worldPos;

    if (def->NORMAL_MAP) {
      glm::vec3 N = glm::normalize(u->u_inverseTransposeModelMatrix * a->a_normal);
      glm::vec3 T = glm::normalize(u->u_inverseTransposeModelMatrix * a->a_tangent);
      v->v_normal = N;
      v->v_tangent = glm::normalize(T - glm::dot(T, N) * N);
    }
  }
};

class FS : public ShaderPbrIBL {
 public:
  CREATE_SHADER_CLONE(FS)

  size_t getSamplerDerivativeOffset(BaseSampler<RGBA> *sampler) const override {
    return offsetof(ShaderVaryings, v_texCoord);
  }

  void setupSamplerDerivative() override {
    if (def->ALBEDO_MAP) {
      u->u_albedoMap->setLodFunc(&texLodFunc);
    }
    if (def->NORMAL_MAP) {
      u->u_normalMap->setLodFunc(&texLodFunc);
    }
    if (def->EMISSIVE_MAP) {
      u->u_emissiveMap->setLodFunc(&texLodFunc);
    }
    if (def->AO_MAP) {
      u->u_aoMap->setLodFunc(&texLodFunc);
    }
  }

  glm::vec3 GetNormalFromMap() {
    if (def->NORMAL_MAP) {
      glm::vec3 N = glm::normalize(v->v_normal);
      glm::vec3 T = glm::normalize(v->v_tangent);
      T = glm::normalize(T - glm::dot(T, N) * N);
      glm::vec3 B = glm::cross(T, N);
      glm::mat3 TBN = glm::mat3(T, B, N);

      glm::vec3 tangentNormal = glm::vec3(texture(u->u_normalMap, v->v_texCoord)) * 2.0f - 1.0f;
      return glm::normalize(TBN * tangentNormal);
    } else {
      return glm::normalize(v->v_normalVector);
    }
  }

  static float DistributionGGX(glm::vec3 N, glm::vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = glm::max(glm::dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / denom;
  }

  static float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
  }

  static float GeometrySmith(glm::vec3 N, glm::vec3 V, glm::vec3 L, float roughness) {
    float NdotV = glm::max(glm::dot(N, V), 0.0f);
    float NdotL = glm::max(glm::dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
  }

  static glm::vec3 FresnelSchlick(float cosTheta, glm::vec3 F0) {
    return F0 + (1.0f - F0) * glm::pow(glm::clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
  }

  static glm::vec3 FresnelSchlickRoughness(float cosTheta, glm::vec3 F0, float roughness) {
    return F0
        + (glm::max(glm::vec3(1.0f - roughness), F0) - F0) * glm::pow(glm::clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
  }

  static glm::vec3 EnvBRDFApprox(glm::vec3 SpecularColor, float Roughness, float NdotV) {
    // [ Lazarov 2013, "Getting More Physical in Call of Duty: Black Ops II" ]
    // Adaptation to fit our G term.
    const glm::vec4 c0 = glm::vec4(-1, -0.0275, -0.572, 0.022);
    const glm::vec4 c1 = glm::vec4(1, 0.0425, 1.04, -0.04);
    glm::vec4 r = Roughness * c0 + c1;
    float a004 = glm::min(r.x * r.x, glm::exp2(-9.28f * NdotV)) * r.x + r.y;
    glm::vec2 AB = glm::vec2(-1.04f, 1.04f) * a004 + glm::vec2(r.z, r.w);

    // Anything less than 2% is physically impossible and is instead considered to be shadowing
    // Note: this is needed for the 'specular' show flag to work, since it uses a SpecularColor of 0
    AB.y *= glm::max(0.f, glm::min(1.f, 50.0f * SpecularColor.g));

    return SpecularColor * AB.x + AB.y;
  }

  void shaderMain() override {
    float pointLightRangeInverse = 1.0f / 5.f;

    glm::vec4 albedo_rgba;
    if (def->ALBEDO_MAP) {
      albedo_rgba = texture(u->u_albedoMap, v->v_texCoord);
    } else {
      albedo_rgba = u->u_baseColor;
    }

    glm::vec3 albedo = glm::pow(glm::vec3(albedo_rgba), glm::vec3(2.2f));

    glm::vec4 metalRoughness = texture(u->u_metalRoughnessMap, v->v_texCoord);
    float metallic = metalRoughness.b;
    float roughness = metalRoughness.g;

    float ao = 1.f;
    if (def->AO_MAP) {
      ao = texture(u->u_aoMap, v->v_texCoord).r;
    }

    glm::vec3 N = GetNormalFromMap();
    glm::vec3 V = glm::normalize(v->v_cameraDirection);
    glm::vec3 R = glm::reflect(-V, N);

    glm::vec3 F0 = glm::vec3(0.04f);
    F0 = glm::mix(F0, albedo, metallic);

    // reflectance equation
    glm::vec3 Lo = glm::vec3(0.0f);

    // Light begin ---------------------------------------------------------------
    if (u->u_enableLight) {
      // calculate per-light radiance
      glm::vec3 L = glm::normalize(v->v_lightDirection);
      glm::vec3 H = glm::normalize(V + L);

      glm::vec3 lDir = v->v_lightDirection * pointLightRangeInverse;
      float attenuation = glm::clamp(1.0f - glm::dot(lDir, lDir), 0.0f, 1.0f);
      glm::vec3 radiance = u->u_pointLightColor * attenuation;

      // Cook-Torrance BRDF
      float NDF = DistributionGGX(N, H, roughness);
      float G = GeometrySmith(N, V, L, roughness);
      glm::vec3 F = FresnelSchlick(glm::max(glm::dot(H, V), 0.0f), F0);

      glm::vec3 numerator = NDF * G * F;
      // + 0.0001 to prevent divide by zero
      float denominator = 4.0f * glm::max(glm::dot(N, V), 0.0f) * glm::max(glm::dot(N, L), 0.0f) + 0.0001f;
      glm::vec3 specular = numerator / denominator;

      glm::vec3 kS = F;
      glm::vec3 kD = glm::vec3(1.0f) - kS;
      kD *= 1.0f - metallic;

      float NdotL = glm::max(glm::dot(N, L), 0.0f);
      Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    // Light end ---------------------------------------------------------------

    // Ambient begin ---------------------------------------------------------------
    glm::vec3 ambient = glm::vec3(0.f);
    if (u->u_enableIBL) {
      glm::vec3 F = FresnelSchlickRoughness(glm::max(glm::dot(N, V), 0.0f), F0, roughness);

      glm::vec3 kS = F;
      glm::vec3 kD = 1.0f - kS;
      kD *= (1.0f - metallic);

      glm::vec3 irradiance = glm::vec3(texture(u->u_irradianceMap, N));
      glm::vec3 diffuse = irradiance * albedo;

      // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
      const float MAX_REFLECTION_LOD = 4.0f;
      glm::vec3 prefilteredColor = glm::vec3(textureLod(u->u_prefilterMap, R, roughness * MAX_REFLECTION_LOD));
      glm::vec3 specular = prefilteredColor * EnvBRDFApprox(F, roughness, glm::max(glm::dot(N, V), 0.0f));
      ambient = (kD * diffuse + specular) * ao;
    } else {
      ambient = u->u_ambientColor * albedo * ao;
    }
    // Ambient end ---------------------------------------------------------------

    glm::vec3 color = ambient + Lo;
    // gamma correct
    color = pow(color, glm::vec3(1.0f / 2.2f));

    // emissive
    glm::vec3 emissive = glm::vec3(0.f);
    if (def->EMISSIVE_MAP) {
      emissive = glm::vec3(texture(u->u_emissiveMap, v->v_texCoord));
    }

    gl->FragColor = glm::vec4(color + emissive, albedo_rgba.a);
  }
};

}
}
