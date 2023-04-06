/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Render/Software/ShaderProgramSoft.h"

namespace SoftGL {
namespace ShaderIBLPrefilter {

struct ShaderDefines {
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

  // UniformsPrefilter
  float u_srcResolution;
  float u_roughness;

  // Samplers
  SamplerCubeSoft<RGBA> *u_cubeMap;
};

struct ShaderVaryings {
  glm::vec3 v_worldPos;
};

class ShaderIBLPrefilter : public ShaderSoft {
 public:
  CREATE_SHADER_OVERRIDE

  std::vector<std::string> &getDefines() override {
    static std::vector<std::string> defines;
    return defines;
  }

  std::vector<UniformDesc> &getUniformsDesc() override {
    static std::vector<UniformDesc> desc = {
        {"UniformsModel", offsetof(ShaderUniforms, u_reverseZ)},
        {"UniformsPrefilter", offsetof(ShaderUniforms, u_srcResolution)},
        {"u_cubeMap", offsetof(ShaderUniforms, u_cubeMap)},
    };
    return desc;
  };
};

class VS : public ShaderIBLPrefilter {
 public:
  CREATE_SHADER_CLONE(VS)

  void shaderMain() override {
    glm::vec4 pos = u->u_modelViewProjectionMatrix * glm::vec4(a->a_position, 1.0);
    gl->Position = pos;
    gl->Position.z = pos.w;
    v->v_worldPos = a->a_position;
  }
};

class FS : public ShaderIBLPrefilter {
 public:
  CREATE_SHADER_CLONE(FS)

  static float DistributionGGX(glm::vec3 N, glm::vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = glm::max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / denom;
  }
  // ----------------------------------------------------------------------------
  // http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
  // efficient VanDerCorpus calculation.
  static float RadicalInverse_VdC(uint32_t bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float((bits) * 2.3283064365386963e-10); // / 0x100000000
  }
  // ----------------------------------------------------------------------------
  static glm::vec2 Hammersley(uint32_t i, uint32_t N) {
    return {float(i) / float(N), RadicalInverse_VdC(i)};
  }
  // ----------------------------------------------------------------------------
  static glm::vec3 ImportanceSampleGGX(glm::vec2 Xi, glm::vec3 N, float roughness) {
    float a = roughness * roughness;

    float phi = 2.0f * PI * Xi.x;
    float cosTheta = glm::sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = glm::sqrt(1.0f - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates - halfway vector
    glm::vec3 H;
    H.x = glm::cos(phi) * sinTheta;
    H.y = glm::sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space H vector to world-space sample vector
    glm::vec3 up = abs(N.z) < 0.999 ? glm::vec3(0.0, 0.0, 1.0) : glm::vec3(1.0, 0.0, 0.0);
    glm::vec3 tangent = glm::normalize(glm::cross(up, N));
    glm::vec3 bitangent = glm::cross(N, tangent);

    glm::vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return glm::normalize(sampleVec);
  }

  void shaderMain() override {
    glm::vec3 N = normalize(v->v_worldPos);

    // make the simplyfying assumption that V equals R equals the normal
    glm::vec3 R = N;
    glm::vec3 V = R;

    const uint32_t SAMPLE_COUNT = 1024u;
    glm::vec3 prefilteredColor = glm::vec3(0.0f);
    float totalWeight = 0.0f;

    for (uint32_t i = 0u; i < SAMPLE_COUNT; ++i) {
      // generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
      glm::vec2 Xi = Hammersley(i, SAMPLE_COUNT);
      glm::vec3 H = ImportanceSampleGGX(Xi, N, u->u_roughness);
      glm::vec3 L = glm::normalize(2.0f * dot(V, H) * H - V);

      float NdotL = glm::max(glm::dot(N, L), 0.0f);
      if (NdotL > 0.0f) {
        // sample from the environment's mip level based on roughness/pdf
        float D = DistributionGGX(N, H, u->u_roughness);
        float NdotH = glm::max(dot(N, H), 0.0f);
        float HdotV = glm::max(dot(H, V), 0.0f);
        float pdf = D * NdotH / (4.0f * HdotV) + 0.0001f;

        float resolution = u->u_srcResolution; // resolution of source cubemap (per face)
        float saTexel = 4.0f * PI / (6.0f * resolution * resolution);
        float saSample = 1.0f / (float(SAMPLE_COUNT) * pdf + 0.0001f);

        float mipLevel = u->u_roughness == 0.0f ? 0.0f : 0.5f * glm::log2(saSample / saTexel);

        prefilteredColor += glm::vec3(textureLod(u->u_cubeMap, L, mipLevel)) * NdotL;
        totalWeight += NdotL;
      }
    }

    prefilteredColor = prefilteredColor / totalWeight;

    gl->FragColor = glm::vec4(prefilteredColor, 1.0);
  }
};

}
}
