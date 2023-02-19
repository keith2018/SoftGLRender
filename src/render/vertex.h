/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <vector>
#include <memory>
#include "base/glm_inc.h"

namespace SoftGL {

enum PrimitiveType {
  Primitive_POINT,
  Primitive_LINE,
  Primitive_TRIANGLE,
};

class VertexArrayObject {
 public:
  virtual int GetId() const = 0;
  virtual void UpdateVertexData(void *data, size_t length) = 0;
};

// only support float type attributes
struct VertexAttributeDesc {
  size_t size;
  size_t stride;
  size_t offset;
};

struct VertexArray {
  std::vector<VertexAttributeDesc> vertexes_desc;
  uint8_t *vertexes_buffer = nullptr;
  size_t vertexes_buffer_length = 0;

  int32_t *indices_buffer = nullptr;
  size_t indices_buffer_length = 0;
};

}
