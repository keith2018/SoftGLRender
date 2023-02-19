/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/vertex.h"

namespace SoftGL {

class VertexArrayObjectSoft : public VertexArrayObject {
 public:
  explicit VertexArrayObjectSoft(const VertexArray &vertex_array)
      : uuid_(uuid_counter_++) {
    // init vertexes
    vertex_stride = vertex_array.vertexes_desc[0].stride;
    vertex_cnt = vertex_array.vertexes_buffer_length / vertex_stride;
    vertexes.resize(vertex_cnt * vertex_stride);
    memcpy(vertexes.data(), vertex_array.vertexes_buffer, vertex_array.vertexes_buffer_length);

    // init indices
    indices_cnt = vertex_array.indices_buffer_length / sizeof(int32_t);
    indices.resize(indices_cnt);
    memcpy(indices.data(), vertex_array.indices_buffer, vertex_array.indices_buffer_length);
  }

  ~VertexArrayObjectSoft() = default;

  void UpdateVertexData(void *data, size_t length) override {
    memcpy(vertexes.data(), data, std::min(length, vertexes.size()));
  }

  int GetId() const override {
    return uuid_;
  }

 public:
  size_t vertex_stride = 0;
  size_t vertex_cnt = 0;
  size_t indices_cnt = 0;
  std::vector<uint8_t> vertexes;
  std::vector<int32_t> indices;

 private:
  int uuid_ = -1;
  static int uuid_counter_;
};

}
