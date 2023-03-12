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
  explicit VertexArrayObjectSoft(const VertexArray &vertexArray)
      : uuid_(uuidCounter_++) {
    // init vertexes
    vertexStride = vertexArray.vertexesDesc[0].stride;
    vertexCnt = vertexArray.vertexesBufferLength / vertexStride;
    vertexes.resize(vertexCnt * vertexStride);
    memcpy(vertexes.data(), vertexArray.vertexesBuffer, vertexArray.vertexesBufferLength);

    // init indices
    indicesCnt = vertexArray.indicesBufferLength / sizeof(int32_t);
    indices.resize(indicesCnt);
    memcpy(indices.data(), vertexArray.indicesBuffer, vertexArray.indicesBufferLength);
  }

  void updateVertexData(void *data, size_t length) override {
    memcpy(vertexes.data(), data, std::min(length, vertexes.size()));
  }

  int getId() const override {
    return uuid_;
  }

 public:
  size_t vertexStride = 0;
  size_t vertexCnt = 0;
  size_t indicesCnt = 0;
  std::vector<uint8_t> vertexes;
  std::vector<int32_t> indices;

 private:
  int uuid_ = -1;
  static int uuidCounter_;
};

}
