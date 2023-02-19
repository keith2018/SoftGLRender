/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/soft/shader_program_soft.h"

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
  // UniformsMVP
  glm::mat4 u_modelMatrix;
  glm::mat4 u_modelViewProjectionMatrix;
  glm::mat3 u_inverseTransposeModelMatrix;

  // UniformsScene
  glm::int32_t u_enablePointLight;
  glm::int32_t u_enableIBL;
  glm::vec3 u_ambientColor;
  glm::vec3 u_cameraPosition;
  glm::vec3 u_pointLightPosition;
  glm::vec3 u_pointLightColor;

  // UniformsColor
  glm::vec4 u_baseColor;

  // Samplers
  Sampler2DSoft *u_albedoMap;
  Sampler2DSoft *u_normalMap;
  Sampler2DSoft *u_emissiveMap;
  Sampler2DSoft *u_aoMap;
};

struct ShaderVaryings {
  glm::vec2 v_texCoord;
  glm::vec3 v_normalVector;
  glm::vec3 v_cameraDirection;
  glm::vec3 v_lightDirection;
};

class ShaderBlinnPhong : public ShaderSoft {
 public:
  CREATE_SHADER_OVERRIDE

  std::vector<std::string> &GetDefines() override {
    static std::vector<std::string> defines = {
        "ALBEDO_MAP",
        "NORMAL_MAP",
        "EMISSIVE_MAP",
        "AO_MAP",
    };
    return defines;
  }

  std::vector<UniformDesc> &GetUniformsDesc() override {
    static std::vector<UniformDesc> desc = {
        {"UniformsMVP", offsetof(ShaderUniforms, u_modelMatrix)},
        {"UniformsScene", offsetof(ShaderUniforms, u_enablePointLight)},
        {"UniformsColor", offsetof(ShaderUniforms, u_baseColor)},
        {"u_albedoMap", offsetof(ShaderUniforms, u_albedoMap)},
        {"u_normalMap", offsetof(ShaderUniforms, u_normalMap)},
        {"u_emissiveMap", offsetof(ShaderUniforms, u_emissiveMap)},
        {"u_aoMap", offsetof(ShaderUniforms, u_aoMap)},
    };
    return desc;
  };
};

class VS : public ShaderBlinnPhong {
 public:
  CREATE_SHADER_CLONE(VS)

  void ShaderMain() override {
    glm::vec4 position = glm::vec4(a->a_position, 1.0);
    gl->Position = u->u_modelViewProjectionMatrix * position;
    v->v_texCoord = a->a_texCoord;

    // world space
    glm::vec3 fragWorldPos = glm::vec3(u->u_modelMatrix * position);
    v->v_normalVector = normalize(u->u_inverseTransposeModelMatrix * a->a_normal);
    v->v_lightDirection = u->u_pointLightPosition - fragWorldPos;
    v->v_cameraDirection = u->u_cameraPosition - fragWorldPos;

    if (def->NORMAL_MAP) {
      // TBN
      glm::vec3 N = normalize(u->u_inverseTransposeModelMatrix * a->a_normal);
      glm::vec3 T = normalize(u->u_inverseTransposeModelMatrix * a->a_tangent);
      T = normalize(T - dot(T, N) * N);
      glm::vec3 B = cross(T, N);
      glm::mat3 TBN = transpose(glm::mat3(T, B, N));

      // TBN space
      v->v_lightDirection = TBN * v->v_lightDirection;
      v->v_cameraDirection = TBN * v->v_cameraDirection;
    }
  }
};

class FS : public ShaderBlinnPhong {
 public:
  CREATE_SHADER_CLONE(FS)

  size_t GetSamplerDerivativeOffset() const override {
    return offsetof(ShaderVaryings, v_texCoord);
  }

  void SetupSamplerDerivative() override {
    if (def->ALBEDO_MAP) {
      u->u_albedoMap->SetLodFunc(&tex_lod_func);
    }
    if (def->NORMAL_MAP) {
      u->u_normalMap->SetLodFunc(&tex_lod_func);
    }
    if (def->EMISSIVE_MAP) {
      u->u_emissiveMap->SetLodFunc(&tex_lod_func);
    }
    if (def->AO_MAP) {
      u->u_aoMap->SetLodFunc(&tex_lod_func);
    }
  }

  glm::vec3 GetNormalFromMap() {
    if (def->NORMAL_MAP) {
      glm::vec3 normalVector = texture(u->u_normalMap, v->v_texCoord);
      return normalize(normalVector * 2.f - 1.f);
    } else {
      return normalize(v->v_normalVector);
    }
  }

  void ShaderMain() override {
    const static float pointLightRangeInverse = 1.0f / 5.f;
    const static float specularExponent = 128.f;

    glm::vec4 baseColor;
    if (def->ALBEDO_MAP) {
      baseColor = texture(u->u_albedoMap, v->v_texCoord);
    } else {
      baseColor = u->u_baseColor;
    }

    glm::vec3 normalVector = GetNormalFromMap();

    // ambient
    float ao = 1.f;
    if (def->AO_MAP) {
      ao = texture(u->u_aoMap, v->v_texCoord).r;
    }
    glm::vec3 ambientColor = glm::vec3(baseColor) * u->u_ambientColor * ao;
    glm::vec3 diffuseColor = glm::vec3(0.f);
    glm::vec3 specularColor = glm::vec3(0.f);
    glm::vec3 emissiveColor = glm::vec3(0.f);

    if (u->u_enablePointLight) {
      // diffuse
      glm::vec3 lDir = v->v_lightDirection * pointLightRangeInverse;
      float attenuation = glm::clamp(1.0f - dot(lDir, lDir), 0.0f, 1.0f);

      glm::vec3 lightDirection = normalize(v->v_lightDirection);
      float diffuse = glm::max(dot(normalVector, lightDirection), 0.0f);
      diffuseColor = u->u_pointLightColor * glm::vec3(baseColor) * diffuse * attenuation;

      // specular
      glm::vec3 cameraDirection = normalize(v->v_cameraDirection);
      glm::vec3 halfVector = normalize(lightDirection + cameraDirection);
      float specularAngle = glm::max(dot(normalVector, halfVector), 0.0f);
      specularColor = glm::vec3(glm::pow(specularAngle, specularExponent));
    }

    if (def->EMISSIVE_MAP) {
      emissiveColor = texture(u->u_emissiveMap, v->v_texCoord);
    }

    gl->FragColor = glm::vec4(ambientColor + diffuseColor + specularColor + emissiveColor, baseColor.a);
  }
};

}
}
