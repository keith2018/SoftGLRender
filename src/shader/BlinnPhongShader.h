/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "renderer/Shader.h"

namespace SoftGL {

#define BlinnPhongShaderAttributes Vertex

struct BlinnPhongShaderUniforms : BaseShaderUniforms {
  // textures
  Sampler2D u_normalMap;
  Sampler2D u_emissiveMap;
  Sampler2D u_diffuseMap;
  Sampler2D u_aoMap;
};

struct BlinnPhongShaderVaryings : BaseShaderVaryings {
  glm::vec2 v_texCoord;
  glm::vec3 v_normalVector;
  glm::vec3 v_cameraDirection;
  glm::vec3 v_lightDirection;
};

struct BlinnPhongVertexShader : BaseVertexShader {

  void shader_main() override {
    auto *a = (BlinnPhongShaderAttributes *) attributes;
    auto *u = (BlinnPhongShaderUniforms *) uniforms;
    auto *v = (BlinnPhongShaderVaryings *) varyings;

    glm::vec4 position = glm::vec4(a->a_position, 1.0f);
    gl_Position = u->u_modelViewProjectionMatrix * position;
    v->v_texCoord = a->a_texCoord;

    glm::vec3 fragWorldPos = glm::vec3(u->u_modelMatrix * position);
    // world space
    v->v_normalVector = glm::normalize(u->u_inverseTransposeModelMatrix * a->a_normal);
    v->v_lightDirection = u->u_pointLightPosition - fragWorldPos;
    v->v_cameraDirection = u->u_cameraPosition - fragWorldPos;

    if (!u->u_normalMap.Empty()) {
      // TBN
      glm::vec3 N = glm::normalize(u->u_inverseTransposeModelMatrix * a->a_normal);
      glm::vec3 T = glm::normalize(u->u_inverseTransposeModelMatrix * a->a_tangent);
      T = glm::normalize(T - glm::dot(T, N) * N);
      glm::vec3 B = glm::cross(T, N);
      glm::mat3 TBN = glm::transpose(glm::mat3(T, B, N));

      // TBN space
      v->v_lightDirection = TBN * v->v_lightDirection;
      v->v_cameraDirection = TBN * v->v_cameraDirection;
    }
  }

  CLONE_VERTEX_SHADER(BlinnPhongVertexShader)
};

struct BlinnPhongFragmentShader : BaseFragmentShader {
  BlinnPhongShaderUniforms *u = nullptr;
  BlinnPhongShaderVaryings *v = nullptr;
  glm::vec3 baseColorRGB{};

  float pointLightRangeInverse = 1.0f / 5.f;
  float specularExponent = 128.f;

  float GetTextureLod(Sampler &sampler) override {
    auto *xa = (BlinnPhongShaderVaryings *) quad_ctx->DFDX_A();
    auto *xb = (BlinnPhongShaderVaryings *) quad_ctx->DFDX_B();
    auto *ya = (BlinnPhongShaderVaryings *) quad_ctx->DFDY_A();
    auto *yb = (BlinnPhongShaderVaryings *) quad_ctx->DFDY_B();
    auto dx = (xa->v_texCoord - xb->v_texCoord) * glm::vec2(sampler.Width(), sampler.Height());
    auto dy = (ya->v_texCoord - yb->v_texCoord) * glm::vec2(sampler.Width(), sampler.Height());
    float d = glm::max(glm::dot(dx, dx), glm::dot(dy, dy));
    return glm::max(0.5f * log2f(d), 0.f);
  }

  glm::vec3 GetNormalFromMap() const {
    if (u->u_normalMap.Empty()) {
      return glm::normalize(v->v_normalVector);
    }

    glm::vec3 normalVector = u->u_normalMap.texture2D(v->v_texCoord);
    return glm::normalize(normalVector * 2.f - 1.f);
  }

  void shader_main() override {
    BaseFragmentShader::shader_main();
    u = (BlinnPhongShaderUniforms *) uniforms;
    v = (BlinnPhongShaderVaryings *) varyings;

    u->u_normalMap.SetLodFunc(&tex_lod_func);
    u->u_emissiveMap.SetLodFunc(&tex_lod_func);
    u->u_diffuseMap.SetLodFunc(&tex_lod_func);
    u->u_aoMap.SetLodFunc(&tex_lod_func);

    glm::vec4 baseColor = u->u_diffuseMap.texture2D(v->v_texCoord);
    baseColorRGB = glm::vec3(baseColor);

    glm::vec3 normalVector = GetNormalFromMap();

    // ambient
    float ao = 1.f;
    if (!u->u_aoMap.Empty()) {
      ao = u->u_aoMap.texture2D(v->v_texCoord).r;
    }
    glm::vec3 ambientColor = baseColorRGB * u->u_ambientColor * ao;

    glm::vec3 diffuseColor = glm::vec3(0.f);
    glm::vec3 specularColor = glm::vec3(0.f);
    if (u->u_showPointLight) {
      // diffuse
      glm::vec3 lDir = v->v_lightDirection * pointLightRangeInverse;
      float attenuation = glm::clamp(1.0f - dot(lDir, lDir), 0.0f, 1.0f);

      glm::vec3 lightDirection = glm::normalize(v->v_lightDirection);
      float diffuse = std::max(dot(normalVector, lightDirection), 0.0f);
      diffuseColor = u->u_pointLightColor * baseColorRGB * diffuse * attenuation;

      // specular
      glm::vec3 cameraDirection = glm::normalize(v->v_cameraDirection);
      glm::vec3 halfVector = glm::normalize(lightDirection + cameraDirection);
      float specularAngle = std::max(glm::dot(normalVector, halfVector), 0.0f);
      specularColor = glm::vec3(glm::pow(specularAngle, specularExponent));
    }

    // emissive
    glm::vec3 emissiveColor = u->u_emissiveMap.texture2D(v->v_texCoord);
    gl_FragColor = glm::vec4(ambientColor + diffuseColor + specularColor + emissiveColor, baseColor.a);

    // alpha discard
    if (u->u_alpha_cutoff >= 0) {
      if (gl_FragColor.a < u->u_alpha_cutoff) {
        discard = true;
      }
    }
  }

  CLONE_FRAGMENT_SHADER(BlinnPhongFragmentShader)
};

}
