/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <vector>
#include "base/glm_inc.h"

namespace SoftGL {

enum PrimitiveType {
  Primitive_POINTS,
  Primitive_LINES,
  Primitive_TRIANGLES,
};

struct Vertex {
  glm::vec3 a_position;
  glm::vec2 a_texCoord;
  glm::vec3 a_normal;
  glm::vec3 a_tangent;
};

class VertexArrayObject {
 public:
  virtual int GetId() const = 0;
  virtual void UpdateVertexData(std::vector<Vertex> &vertexes) = 0;
  virtual void Bind() = 0;
};

class VertexArray {
 public:
  inline void UpdateVertexData() {
    if (vao) { vao->UpdateVertexData(vertexes); }
  };

 public:
  PrimitiveType primitive_type;
  size_t primitive_cnt = 0;
  std::vector<Vertex> vertexes;
  std::vector<int32_t> indices;

  std::shared_ptr<VertexArrayObject> vao = nullptr;
};

}
