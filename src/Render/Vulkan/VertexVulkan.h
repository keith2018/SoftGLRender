/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/UUID.h"
#include "Render/Vertex.h"
#include "VulkanUtils.h"

namespace SoftGL {

class VertexArrayObjectVulkan : public VertexArrayObject {
 public:
  VertexArrayObjectVulkan(VKContext &ctx, const VertexArray &vertexArr)
      : vkCtx_(ctx) {
    device_ = ctx.device();
    if (!vertexArr.vertexesBuffer || !vertexArr.indexBuffer) {
      return;
    }
    indicesCnt_ = vertexArr.indexBufferLength / sizeof(int32_t);

    // init vertex input info
    bindingDescription_.binding = 0;
    bindingDescription_.stride = vertexArr.vertexSize;
    bindingDescription_.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    size_t attrCnt = vertexArr.vertexesDesc.size();
    attributeDescriptions_.resize(attrCnt);
    for (size_t i = 0; i < attrCnt; i++) {
      auto &attrDesc = vertexArr.vertexesDesc[i];
      attributeDescriptions_[i].binding = 0;
      attributeDescriptions_[i].location = i;
      attributeDescriptions_[i].format = vertexAttributeFormat(attrDesc.size);
      attributeDescriptions_[i].offset = attrDesc.offset;
    }

    vertexInputInfo_.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo_.vertexBindingDescriptionCount = 1;
    vertexInputInfo_.pVertexBindingDescriptions = &bindingDescription_;
    vertexInputInfo_.vertexAttributeDescriptionCount = attributeDescriptions_.size();
    vertexInputInfo_.pVertexAttributeDescriptions = attributeDescriptions_.data();

    // create buffers
    VulkanUtils::createBuffer(vkCtx_,
                              vertexArr.vertexesBufferLength,
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              vertexBuffer_,
                              vertexBufferMemory_);

    VulkanUtils::createBuffer(vkCtx_,
                              vertexArr.indexBufferLength,
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              indexBuffer_,
                              indexBufferMemory_);

    // upload data
    VulkanUtils::uploadBufferData(vkCtx_, vertexBuffer_, vertexArr.vertexesBuffer, vertexArr.vertexesBufferLength);
    VulkanUtils::uploadBufferData(vkCtx_, indexBuffer_, vertexArr.indexBuffer, vertexArr.indexBufferLength);
  }

  ~VertexArrayObjectVulkan() {
    vkDestroyBuffer(device_, vertexBuffer_, nullptr);
    vkFreeMemory(device_, vertexBufferMemory_, nullptr);

    vkDestroyBuffer(device_, indexBuffer_, nullptr);
    vkFreeMemory(device_, indexBufferMemory_, nullptr);
  }

  int getId() const override {
    return uuid_.get();
  }

  void updateVertexData(void *data, size_t length) override {
    VulkanUtils::uploadBufferData(vkCtx_, vertexBuffer_, data, length);
  }

  // only Float element
  static VkFormat vertexAttributeFormat(size_t size) {
    switch (size) {
      case 1: return VK_FORMAT_R32_SFLOAT;
      case 2: return VK_FORMAT_R32G32_SFLOAT;
      case 3: return VK_FORMAT_R32G32B32_SFLOAT;
      case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
      default:
        break;
    }
    return VK_FORMAT_UNDEFINED;
  }

  inline VkPipelineVertexInputStateCreateInfo &getVertexInputInfo() {
    return vertexInputInfo_;
  }

  inline uint32_t getIndicesCnt() const {
    return indicesCnt_;
  }

  inline VkBuffer &getVertexBuffer() {
    return vertexBuffer_;
  }

  inline VkBuffer &getIndexBuffer() {
    return indexBuffer_;
  }

 private:
  UUID<VertexArrayObjectVulkan> uuid_;
  uint32_t indicesCnt_ = 0;

  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  VkPipelineVertexInputStateCreateInfo vertexInputInfo_{};
  VkVertexInputBindingDescription bindingDescription_{};
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions_;

  VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
  VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
  VkBuffer indexBuffer_ = VK_NULL_HANDLE;
  VkDeviceMemory indexBufferMemory_ = VK_NULL_HANDLE;
};

}
