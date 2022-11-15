/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "../shader.h"

namespace SoftGL {

#define PrefilterShaderAttributes Vertex

struct PrefilterShaderUniforms : BaseShaderUniforms {
  SamplerCube u_cubeMap;
  float u_srcResolution;
  float u_roughness;
};

struct PrefilterShaderVaryings : BaseShaderVaryings {
  glm::vec3 v_texCoord;
};

struct PrefilterVertexShader : BaseVertexShader {

  void shader_main() override {
    auto *a = (PrefilterShaderAttributes *) attributes;
    auto *u = (PrefilterShaderUniforms *) uniforms;
    auto *v = (PrefilterShaderVaryings *) varyings;

    glm::vec4 position = glm::vec4(a->a_position, 1.0f);
    gl_Position = u->u_modelViewProjectionMatrix * position;
    gl_Position.z = gl_Position.w;  // depth -> 1.0

    v->v_texCoord = a->a_position;
  }

  CLONE_VERTEX_SHADER(PrefilterVertexShader)
};

struct PrefilterFragmentShader : BaseFragmentShader {
  PrefilterShaderUniforms *u = nullptr;
  PrefilterShaderVaryings *v = nullptr;

  float DistributionGGX(glm::vec3 N, glm::vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = std::max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / denom;
  }
  // ----------------------------------------------------------------------------
  // http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
  // efficient VanDerCorpus calculation.
  float RadicalInverse_VdC(uint32_t bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
  }
  // ----------------------------------------------------------------------------
  glm::vec2 Hammersley(uint32_t i, uint32_t N) {
    return glm::vec2(float(i) / float(N), RadicalInverse_VdC(i));
  }
  // ----------------------------------------------------------------------------
  glm::vec3 ImportanceSampleGGX(glm::vec2 Xi, glm::vec3 N, float roughness) {
    float a = roughness * roughness;

    float phi = 2.0f * PI * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates - halfway vector
    glm::vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space H vector to world-space sample vector
    glm::vec3 up = abs(N.z) < 0.999 ? glm::vec3(0.0, 0.0, 1.0) : glm::vec3(1.0, 0.0, 0.0);
    glm::vec3 tangent = glm::normalize(glm::cross(up, N));
    glm::vec3 bitangent = glm::cross(N, tangent);

    glm::vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return glm::normalize(sampleVec);
  }

  void shader_main() override {
    BaseFragmentShader::shader_main();
    u = (PrefilterShaderUniforms *) uniforms;
    v = (PrefilterShaderVaryings *) varyings;

    glm::vec3 N = normalize(v->v_texCoord);

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

      float NdotL = std::max(glm::dot(N, L), 0.0f);
      if (NdotL > 0.0f) {
        // sample from the environment's mip level based on roughness/pdf
        float D = DistributionGGX(N, H, u->u_roughness);
        float NdotH = std::max(dot(N, H), 0.0f);
        float HdotV = std::max(dot(H, V), 0.0f);
        float pdf = D * NdotH / (4.0f * HdotV) + 0.0001f;

        float resolution = u->u_srcResolution; // resolution of source cubemap (per face)
        float saTexel = 4.0f * PI / (6.0f * resolution * resolution);
        float saSample = 1.0f / (float(SAMPLE_COUNT) * pdf + 0.0001f);

        float mipLevel = u->u_roughness == 0.0f ? 0.0f : 0.5f * log2(saSample / saTexel);

        prefilteredColor += glm::vec3(u->u_cubeMap.textureCubeLod(L, mipLevel)) * NdotL;
        totalWeight += NdotL;
      }
    }

    prefilteredColor = prefilteredColor / totalWeight;

    gl_FragColor = glm::vec4(prefilteredColor, 1.0);
  }

  CLONE_FRAGMENT_SHADER(PrefilterFragmentShader)
};

}
