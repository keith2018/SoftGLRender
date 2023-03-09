/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/soft/shader_program_soft.h"

namespace SoftGL {
namespace ShaderBasic {

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
  glm::mat4 u_modelMatrix;
  glm::mat4 u_modelViewProjectionMatrix;
  glm::mat3 u_inverseTransposeModelMatrix;
  glm::mat4 u_shadowMVPMatrix;

  // UniformsMaterial
  glm::int32_t u_enableLight;
  glm::int32_t u_enableIBL;
  glm::int32_t u_enableShadow;
  glm::int32_t u_kSpecular;
  glm::vec4 u_baseColor;
};

struct ShaderVaryings {
};

class ShaderBasic : public ShaderSoft {
 public:
  CREATE_SHADER_OVERRIDE

  std::vector<std::string> &GetDefines() override {
    static std::vector<std::string> defines;
    return defines;
  }

  std::vector<UniformDesc> &GetUniformsDesc() override {
    static std::vector<UniformDesc> desc = {
        {"UniformsModel", offsetof(ShaderUniforms, u_modelMatrix)},
        {"UniformsMaterial", offsetof(ShaderUniforms, u_enableLight)},
    };
    return desc;
  };
};

class VS : public ShaderBasic {
 public:
  CREATE_SHADER_CLONE(VS)

  void ShaderMain() override {
    gl->Position = u->u_modelViewProjectionMatrix * glm::vec4(a->a_position, 1.0);
  }
};

class FS : public ShaderBasic {
 public:
  CREATE_SHADER_CLONE(FS)

  void ShaderMain() override {
    gl->FragColor = u->u_baseColor;
  }
};

}
}
