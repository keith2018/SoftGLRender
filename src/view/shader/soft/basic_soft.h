/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/soft/shader_program_soft.h"

namespace SoftGL {
namespace ShaderBasic {

struct VertexAttributes {
  glm::vec3 a_position;
  glm::vec2 a_texCoord;
  glm::vec3 a_normal;
  glm::vec3 a_tangent;
};

struct UniformBlocks {
  // UniformsMVP
  glm::mat4 u_modelMatrix;
  glm::mat4 u_modelViewProjectionMatrix;
  glm::mat3 u_inverseTransposeModelMatrix;

  // UniformsColor
  glm::vec4 u_baseColor;
};

class ShaderBasic : public ShaderSoft {
 public:
  void BindBuiltin(void *ptr) override {
    gl = static_cast<ShaderBuiltin *>(ptr);
  }

  void BindVertexAttributes(void *ptr) override {
    a = static_cast<VertexAttributes *>(ptr);
  }

  void BindUniformBlocks(void *ptr) override {
    u = static_cast<UniformBlocks *>(ptr);
  }

  std::vector<UniformBlockDesc> &GetUniformBlockDesc() override {
    static std::vector<UniformBlockDesc> desc = {
        {"UniformsMVP", offsetof(UniformBlocks, u_modelMatrix)},
        {"UniformsColor", offsetof(UniformBlocks, u_baseColor)},
    };
    return desc;
  };

  size_t GetUniformBlocksSize() override {
    return sizeof(UniformBlocks);
  }

 public:
  ShaderBuiltin *gl = nullptr;
  VertexAttributes *a = nullptr;
  UniformBlocks *u = nullptr;
};

class VS_BASIC : public ShaderBasic {
 public:
  void ShaderMain() override {
    gl->Position = u->u_modelViewProjectionMatrix * glm::vec4(a->a_position, 1.0);
  }
};

class FS_BASIC : public ShaderBasic {
 public:
  void ShaderMain() override {
    gl->FragColor = u->u_baseColor;
  }
};

}
}
