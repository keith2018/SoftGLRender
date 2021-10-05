/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "renderer/Shader.h"

namespace SoftGL {

#define PBRShaderAttributes Vertex

struct PBRShaderUniforms : BaseShaderUniforms {
  // textures
  Sampler2D u_normalMap;
  Sampler2D u_emissiveMap;
  Sampler2D u_albedoMap;
  Sampler2D u_metalRoughnessMap;
  Sampler2D u_aoMap;

  // IBL
  SamplerCube u_irradianceMap;
  SamplerCube u_prefilterMapMap;
  Sampler2D u_brdfLutMap;
};

struct PBRShaderVaryings : BaseShaderVaryings {
  glm::vec2 v_texCoord;
  glm::vec3 v_normal;
  glm::vec3 v_worldPos;

  glm::vec3 v_cameraDirection;
  glm::vec3 v_lightDirection;
};

struct PBRVertexShader : BaseVertexShader {

  void shader_main() override {
    auto *a = (PBRShaderAttributes *) attributes;
    auto *u = (PBRShaderUniforms *) uniforms;
    auto *v = (PBRShaderVaryings *) varyings;

    glm::vec4 position = glm::vec4(a->a_position, 1.0f);
    gl_Position = u->u_modelViewProjectionMatrix * position;

    v->v_texCoord = a->a_texCoord;
    v->v_worldPos = glm::vec3(u->u_modelMatrix * position);
    v->v_normal = glm::mat3(u->u_modelMatrix) * a->a_normal;

    // world space
    v->v_lightDirection = u->u_pointLightPosition - v->v_worldPos;
    v->v_cameraDirection = u->u_cameraPosition - v->v_worldPos;
  }

  CLONE_VERTEX_SHADER(PBRVertexShader)
};

struct PBRFragmentShader : BaseFragmentShader {
  PBRShaderUniforms *u = nullptr;
  PBRShaderVaryings *v = nullptr;

  float pointLightRangeInverse = 1.0f / 5.f;

  glm::vec3 GetNormalFromMap() const {
    glm::vec3 N = normalize(v->v_normal);
    if (u->u_normalMap.Empty()) {
      return N;
    }

    glm::vec3 tangentNormal = u->u_normalMap.texture2D(v->v_texCoord) * 2.f - 1.f;

    // TBN -> World
    auto *xa = (PBRShaderVaryings *) quad_ctx->DFDX_A();
    auto *xb = (PBRShaderVaryings *) quad_ctx->DFDX_B();
    auto *ya = (PBRShaderVaryings *) quad_ctx->DFDY_A();
    auto *yb = (PBRShaderVaryings *) quad_ctx->DFDY_B();

    glm::vec3 Q1 = xa->v_worldPos - xb->v_worldPos;
    glm::vec3 Q2 = ya->v_worldPos - yb->v_worldPos;
    glm::vec2 st1 = xa->v_texCoord - xb->v_texCoord;
    glm::vec2 st2 = ya->v_texCoord - yb->v_texCoord;

    glm::vec3 T = glm::normalize(Q1 * st2.t - Q2 * st1.t);
    glm::vec3 B = -glm::normalize(glm::cross(N, T));
    glm::mat3 TBN = glm::mat3(T, B, N);

    return glm::normalize(TBN * tangentNormal);
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
    return F0 + (1.0f - F0) * pow(glm::clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
  }

  static glm::vec3 FresnelSchlickRoughness(float cosTheta, glm::vec3 F0, float roughness) {
    return F0 + (max(glm::vec3(1.0f - roughness), F0) - F0) * pow(glm::clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
  }

  static glm::vec3 EnvBRDFApprox(glm::vec3 SpecularColor, float Roughness, float NdotV) {
    // [ Lazarov 2013, "Getting More Physical in Call of Duty: Black Ops II" ]
    // Adaptation to fit our G term.
    const glm::vec4 c0 = {-1, -0.0275, -0.572, 0.022};
    const glm::vec4 c1 = {1, 0.0425, 1.04, -0.04};
    glm::vec4 r = Roughness * c0 + c1;
    float a004 = glm::min(r.x * r.x, glm::exp2(-9.28f * NdotV)) * r.x + r.y;
    glm::vec2 AB = glm::vec2(-1.04f, 1.04f) * a004 + glm::vec2(r.z, r.w);

    // Anything less than 2% is physically impossible and is instead considered to be shadowing
    // Note: this is needed for the 'specular' show flag to work, since it uses a SpecularColor of 0
    AB.y *= glm::max(0.f, glm::min(1.f, 50.0f * SpecularColor.g));

    return SpecularColor * AB.x + AB.y;
  }

  void shader_main() override {
    BaseFragmentShader::shader_main();
    u = (PBRShaderUniforms *) uniforms;
    v = (PBRShaderVaryings *) varyings;

    glm::vec4 albedo_rgba = u->u_albedoMap.texture2D(v->v_texCoord);
    glm::vec3 albedo = glm::pow(glm::vec3(albedo_rgba), glm::vec3(2.2f));

    glm::vec4 metalRoughness = u->u_metalRoughnessMap.texture2D(v->v_texCoord);
    float metallic = metalRoughness.b;
    float roughness = metalRoughness.g;

    float ao = 1.f;
    if (!u->u_aoMap.Empty()) {
      ao = u->u_aoMap.texture2D(v->v_texCoord).r;
    }

    glm::vec3 N = GetNormalFromMap();
    glm::vec3 V = glm::normalize(v->v_cameraDirection);
    glm::vec3 R = glm::reflect(-V, N);

    glm::vec3 F0 = glm::vec3(0.04f);
    F0 = glm::mix(F0, albedo, metallic);

    // reflectance equation
    glm::vec3 Lo = glm::vec3(0.0f);

    // Light begin ---------------------------------------------------------------
    if (u->u_showPointLight) {
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
      Lo += (kD * albedo / glm::pi<float>() + specular) * radiance * NdotL;
    }
    // Light end ---------------------------------------------------------------

    // Ambient begin ---------------------------------------------------------------
    glm::vec3 ambient(0.f);
    if (u->u_irradianceMap.Empty() || u->u_prefilterMapMap.Empty()) {
      ambient = u->u_ambientColor * albedo * ao;
    } else {
      glm::vec3 F = FresnelSchlickRoughness(glm::max(glm::dot(N, V), 0.0f), F0, roughness);

      glm::vec3 kS = F;
      glm::vec3 kD = 1.0f - kS;
      kD *= (1.0f - metallic);

      glm::vec3 irradiance = glm::vec3(u->u_irradianceMap.textureCube(N));
      glm::vec3 diffuse = irradiance * albedo;

      // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
      const float MAX_REFLECTION_LOD = 4.0f;
      glm::vec3 prefilteredColor = glm::vec3(u->u_prefilterMapMap.textureCubeLod(R, roughness * MAX_REFLECTION_LOD));
#ifndef SOFTGL_BRDF_APPROX
      glm::vec4 brdf = u->u_brdfLutMap.texture2D(glm::vec2(glm::max(glm::dot(N, V), 0.0f), roughness));
      specular = prefilteredColor * (F * brdf.x + brdf.y);
#else
      glm::vec3 specular = prefilteredColor * EnvBRDFApprox(F, roughness, glm::max(glm::dot(N, V), 0.0f));
#endif
      ambient = (kD * diffuse + specular) * ao;
    }
    // Ambient end ---------------------------------------------------------------

    glm::vec3 color = ambient + Lo;
    // gamma correct
    color = pow(color, glm::vec3(1.0f / 2.2f));

    // emissive
    glm::vec3 emissive = u->u_emissiveMap.texture2D(v->v_texCoord);
    gl_FragColor = glm::vec4(color + emissive, albedo_rgba.a);

    // alpha discard
    if (u->u_alpha_cutoff >= 0) {
      if (gl_FragColor.a < u->u_alpha_cutoff) {
        discard = true;
      }
    }
  }

  CLONE_FRAGMENT_SHADER(PBRFragmentShader)
};

}
