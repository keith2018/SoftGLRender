/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/UUID.h"
#include "Render/Vertex.h"
#include "Render/Vulkan/VKContext.h"

namespace SoftGL {

class VertexArrayObjectVulkan : public VertexArrayObject {
 public:
  VertexArrayObjectVulkan(VKContext &ctx, const VertexArray &vertex_array)
      : vkCtx_(ctx) {
    if (!vertex_array.vertexesBuffer || !vertex_array.indicesBuffer) {
      return;
    }
    device_ = ctx.getDevice();
    indicesCnt_ = vertex_array.indicesBufferLength / sizeof(int32_t);
  }

  int getId() const override {
    return uuid_.get();
  }

  void updateVertexData(void *data, size_t length) override {
  }

  size_t getIndicesCnt() const {
    return indicesCnt_;
  }

 private:
  UUID<VertexArrayObjectVulkan> uuid_;
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  size_t indicesCnt_ = 0;
};

}
