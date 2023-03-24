/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/Logger.h"
#include "VKContext.h"

namespace SoftGL {

#ifdef DEBUG
#define VK_CHECK(stmt) do {                                                                 \
            VkResult result = (stmt);                                                       \
            if (result != VK_SUCCESS) {                                                     \
              LOGE("VK_CHECK: VkResult: %d, %s:%d, %s", result, __FILE__, __LINE__, #stmt); \
              abort();                                                                      \
            }                                                                               \
        } while (0)
#else
#define VK_CHECK(stmt) stmt
#endif

class VulkanUtils {
 public:
  static void createBuffer(VKContext &ctx,
                           VkDeviceSize size,
                           VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties,
                           VkBuffer &buffer,
                           VkDeviceMemory &bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateBuffer(ctx.device(), &bufferInfo, nullptr, &buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(ctx.device(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = ctx.getMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

    VK_CHECK(vkAllocateMemory(ctx.device(), &allocInfo, nullptr, &bufferMemory));
    VK_CHECK(vkBindBufferMemory(ctx.device(), buffer, bufferMemory, 0));
  }

  static void copyBuffer(VKContext &ctx, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = ctx.beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    ctx.endSingleTimeCommands(commandBuffer);
  }

  static void uploadBufferData(VKContext &ctx, VkBuffer &buffer, void *bufferData, VkDeviceSize bufferSize) {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(ctx, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void *dataPtr = nullptr;
    VK_CHECK(vkMapMemory(ctx.device(), stagingBufferMemory, 0, bufferSize, 0, &dataPtr));
    memcpy(dataPtr, bufferData, (size_t) bufferSize);
    vkUnmapMemory(ctx.device(), stagingBufferMemory);

    copyBuffer(ctx, stagingBuffer, buffer, bufferSize);

    vkDestroyBuffer(ctx.device(), stagingBuffer, nullptr);
    vkFreeMemory(ctx.device(), stagingBufferMemory, nullptr);
  }

  static void submitWork(VkCommandBuffer cmdBuffer, VkQueue queue, VkFence fence) {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, fence));
  }
};

}
