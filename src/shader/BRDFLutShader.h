/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "renderer/Shader.h"

namespace SoftGL {

#define BRDFLutShaderAttributes Vertex

struct BRDFLutShaderUniforms : BaseShaderUniforms {
};

struct BRDFLutShaderVaryings : BaseShaderVaryings {
  glm::vec2 v_texCoord;
};

struct BRDFLutVertexShader : BaseVertexShader {

  void shader_main() override {
    auto *a = (BRDFLutShaderAttributes *) attributes;
    auto *v = (BRDFLutShaderVaryings *) varyings;

    gl_Position = glm::vec4(a->a_position, 1.0f);

    v->v_texCoord = a->a_texCoord;
  }

  CLONE_VERTEX_SHADER(BRDFLutVertexShader)
};

struct BRDFLutFragmentShader : BaseFragmentShader {
  BRDFLutShaderUniforms *u = nullptr;
  BRDFLutShaderVaryings *v = nullptr;

  // ----------------------------------------------------------------------------
  // http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
  // efficient VanDerCorpus calculation.
  float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
  }
  // ----------------------------------------------------------------------------
  glm::vec2 Hammersley(uint i, uint N) {
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
    glm::vec3 up = abs(N.z) < 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 tangent = glm::normalize(glm::cross(up, N));
    glm::vec3 bitangent = glm::cross(N, tangent);

    glm::vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return glm::normalize(sampleVec);
  }
  // ----------------------------------------------------------------------------
  float GeometrySchlickGGX(float NdotV, float roughness) {
    // note that we use a different k for IBL
    float a = roughness;
    float k = (a * a) / 2.0f;

    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
  }
  // ----------------------------------------------------------------------------
  float GeometrySmith(glm::vec3 N, glm::vec3 V, glm::vec3 L, float roughness) {
    float NdotV = glm::max(glm::dot(N, V), 0.0f);
    float NdotL = glm::max(glm::dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
  }
  // ----------------------------------------------------------------------------
  glm::vec2 IntegrateBRDF(float NdotV, float roughness) {
    glm::vec3 V;
    V.x = sqrt(1.0f - NdotV * NdotV);
    V.y = 0.0f;
    V.z = NdotV;

    float A = 0.0f;
    float B = 0.0f;

    glm::vec3 N = glm::vec3(0.0f, 0.0f, 1.0f);

    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
      // generates a sample vector that's biased towards the
      // preferred alignment direction (importance sampling).
      glm::vec2 Xi = Hammersley(i, SAMPLE_COUNT);
      glm::vec3 H = ImportanceSampleGGX(Xi, N, roughness);
      glm::vec3 L = normalize(2.0f * dot(V, H) * H - V);

      float NdotL = glm::max(L.z, 0.0f);
      float NdotH = glm::max(H.z, 0.0f);
      float VdotH = glm::max(dot(V, H), 0.0f);

      if (NdotL > 0.0f) {
        float G = GeometrySmith(N, V, L, roughness);
        float G_Vis = (G * VdotH) / (NdotH * NdotV);
        float Fc = std::pow(1.0f - VdotH, 5.0f);

        A += (1.0f - Fc) * G_Vis;
        B += Fc * G_Vis;
      }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return glm::vec2(A, B);
  }

  void shader_main() override {
    BaseFragmentShader::shader_main();
    u = (BRDFLutShaderUniforms *) uniforms;
    v = (BRDFLutShaderVaryings *) varyings;

    glm::vec2 brdf = IntegrateBRDF(v->v_texCoord.x, v->v_texCoord.y);
    gl_FragColor = glm::vec4(brdf.x, brdf.y, 0.1f, 1.f);
  }

  CLONE_FRAGMENT_SHADER(BRDFLutFragmentShader)
};

}
