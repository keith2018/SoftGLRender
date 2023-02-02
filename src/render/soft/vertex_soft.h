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
  explicit VertexArrayObjectSoft(const VertexArray &vertex_array) : uuid_(uuid_counter_++) {
  }

  ~VertexArrayObjectSoft() = default;

  void UpdateVertexData(std::vector<Vertex> &vertexes) override {
  }

  int GetId() const override {
    return uuid_;
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
};

}