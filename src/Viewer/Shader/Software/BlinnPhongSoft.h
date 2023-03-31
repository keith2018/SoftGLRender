/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Render/Software/ShaderProgramSoft.h"

namespace SoftGL {
namespace ShaderBlinnPhong {

struct ShaderDefines {
  uint8_t ALBEDO_MAP;
  uint8_t NORMAL_MAP;
  uint8_t EMISSIVE_MAP;
  uint8_t AO_MAP;
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
  glm::float32_t u_pointSize;
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
  Sampler2DSoft<float> *u_shadowMap;
};

struct ShaderVaryings {
  glm::vec2 v_texCoord;
  glm::vec3 v_normalVector;
  glm::vec3 v_worldPos;
  glm::vec3 v_cameraDirection;
  glm::vec3 v_lightDirection;
  glm::vec4 v_shadowFragPos;

  glm::vec3 v_normal;
  glm::vec3 v_tangent;
};

class ShaderBlinnPhong : public ShaderSoft {
 public:
  CREATE_SHADER_OVERRIDE

  std::vector<std::string> &getDefines() override {
    static std::vector<std::string> defines = {
        "ALBEDO_MAP",
        "NORMAL_MAP",
        "EMISSIVE_MAP",
        "AO_MAP",
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
        {"u_shadowMap", offsetof(ShaderUniforms, u_shadowMap)},
    };
    return desc;
  };
};

class VS : public ShaderBlinnPhong {
 public:
  CREATE_SHADER_CLONE(VS)

  void shaderMain() override {
    glm::vec4 position = glm::vec4(a->a_position, 1.0);
    gl->Position = u->u_modelViewProjectionMatrix * position;
    v->v_texCoord = a->a_texCoord;
    v->v_shadowFragPos = u->u_shadowMVPMatrix * position;

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

class FS : public ShaderBlinnPhong {
 public:
  CREATE_SHADER_CLONE(FS)

  const float depthBiasCoeff = 0.00025f;
  const float depthBiasMin = 0.00005f;

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

  float ShadowCalculation(glm::vec4 fragPos, glm::vec3 normal) {
    glm::vec3 projCoords = glm::vec3(fragPos) / fragPos.w;
    float currentDepth = projCoords.z;
    if (currentDepth < 0.f || currentDepth > 1.f) {
      return 0.0f;
    }

    float bias = glm::max(depthBiasCoeff * (1.0f - glm::dot(normal, glm::normalize(v->v_lightDirection))), depthBiasMin);
    float shadow = 0.0;

    // PCF
    glm::vec2 pixelOffset = 1.0f / (glm::vec2) textureSize(u->u_shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
      for (int y = -1; y <= 1; ++y) {
        float pcfDepth = texture(u->u_shadowMap, glm::vec2(projCoords) + glm::vec2(x, y) * pixelOffset);
        if (u->u_reverseZ) {
          pcfDepth = 1.f - pcfDepth;
        }
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
      }
    }
    shadow /= 9.0;
    return shadow;
  }

  void shaderMain() override {
    const static float pointLightRangeInverse = 1.0f / 5.f;
    const static float specularExponent = 128.f;

    glm::vec4 baseColor;
    if (def->ALBEDO_MAP) {
      baseColor = texture(u->u_albedoMap, v->v_texCoord);
    } else {
      baseColor = u->u_baseColor;
    }

    glm::vec3 N = GetNormalFromMap();

    // ambient
    float ao = 1.f;
    if (def->AO_MAP) {
      ao = texture(u->u_aoMap, v->v_texCoord).r;
    }
    glm::vec3 ambientColor = glm::vec3(baseColor) * u->u_ambientColor * ao;
    glm::vec3 diffuseColor = glm::vec3(0.f);
    glm::vec3 specularColor = glm::vec3(0.f);
    glm::vec3 emissiveColor = glm::vec3(0.f);

    if (u->u_enableLight) {
      // diffuse
      glm::vec3 lDir = v->v_lightDirection * pointLightRangeInverse;
      float attenuation = glm::clamp(1.0f - dot(lDir, lDir), 0.0f, 1.0f);

      glm::vec3 lightDirection = normalize(v->v_lightDirection);
      float diffuse = glm::max(dot(N, lightDirection), 0.0f);
      diffuseColor = u->u_pointLightColor * glm::vec3(baseColor) * diffuse * attenuation;

      // specular
      glm::vec3 cameraDirection = normalize(v->v_cameraDirection);
      glm::vec3 halfVector = normalize(lightDirection + cameraDirection);
      float specularAngle = glm::max(dot(N, halfVector), 0.0f);
      specularColor = u->u_kSpecular * glm::vec3(glm::pow(specularAngle, specularExponent));

      if (u->u_enableShadow) {
        // calculate shadow
        float shadow = 1.0f - ShadowCalculation(v->v_shadowFragPos, N);
        diffuseColor *= shadow;
        specularColor *= shadow;
      }
    }

    if (def->EMISSIVE_MAP) {
      emissiveColor = texture(u->u_emissiveMap, v->v_texCoord);
    }

    gl->FragColor = glm::vec4(ambientColor + diffuseColor + specularColor + emissiveColor, baseColor.a);
  }
};

}
}
