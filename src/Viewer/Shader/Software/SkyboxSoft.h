/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Render/Software/ShaderProgramSoft.h"

namespace SoftGL {
namespace ShaderSkybox {

struct ShaderDefines {
  uint8_t EQUIRECTANGULAR_MAP;
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

  // Samplers
  Sampler2DSoft<RGBA> *u_equirectangularMap;
  SamplerCubeSoft<RGBA> *u_cubeMap;
};

struct ShaderVaryings {
  glm::vec3 v_worldPos;
};

class ShaderSkybox : public ShaderSoft {
 public:
  CREATE_SHADER_OVERRIDE

  std::vector<std::string> &getDefines() override {
    static std::vector<std::string> defines = {
        "EQUIRECTANGULAR_MAP",
    };
    return defines;
  }

  std::vector<UniformDesc> &getUniformsDesc() override {
    static std::vector<UniformDesc> desc = {
        {"UniformsModel", offsetof(ShaderUniforms, u_reverseZ)},
        {"u_equirectangularMap", offsetof(ShaderUniforms, u_equirectangularMap)},
        {"u_cubeMap", offsetof(ShaderUniforms, u_cubeMap)},
    };
    return desc;
  };
};

class VS : public ShaderSkybox {
 public:
  CREATE_SHADER_CLONE(VS)

  void shaderMain() override {
    glm::vec4 pos = u->u_modelViewProjectionMatrix * glm::vec4(a->a_position, 1.0f);
    gl->Position = glm::vec4(pos.x, pos.y, pos.w, pos.w);

    if (u->u_reverseZ) {
      gl->Position.z = 0.f;
    }

    v->v_worldPos = a->a_position;
  }
};

class FS : public ShaderSkybox {
 public:
  CREATE_SHADER_CLONE(FS)

  static glm::vec2 SampleSphericalMap(glm::vec3 dir) {
    glm::vec2 uv = glm::vec2(glm::atan(dir.z, dir.x), asin(-dir.y));
    uv *= glm::vec2(0.1591f, 0.3183f);
    uv += 0.5f;
    return uv;
  }

  void shaderMain() override {
    if (def->EQUIRECTANGULAR_MAP) {
      glm::vec2 uv = SampleSphericalMap(normalize(v->v_worldPos));
      gl->FragColor = texture(u->u_equirectangularMap, uv);
    } else {
      gl->FragColor = texture(u->u_cubeMap, v->v_worldPos);
    }
  }
};

}
}
