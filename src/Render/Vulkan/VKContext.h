/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <string>
#include <vector>
#include "VulkanInc.h"

namespace SoftGL {

struct QueueFamilyIndices {
  int32_t graphicsFamily = -1;

  bool isComplete() const {
    return graphicsFamily >= 0;
  }
};

class VKContext {
 public:
  bool create(bool debugOutput = false);
  void destroy();

  inline VkDevice &device() {
    return device_;
  }

  inline VkQueue &getGraphicsQueue() {
    return graphicsQueue_;
  }

  inline VkCommandPool &getCommandPool() {
    return commandPool_;
  }

  inline VkPhysicalDeviceProperties &getPhysicalDeviceProperties() {
    return deviceProperties_;
  }

  uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);

  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);

  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                    VkBuffer &buffer, VkDeviceMemory &bufferMemory);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  void uploadBufferData(VkBuffer &buffer, void *bufferData, VkDeviceSize bufferSize);

  void submitWork(VkCommandBuffer cmdBuffer, VkFence fence);

  static void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags imageAspect,
                                    VkImageLayout oldLayout, VkImageLayout newLayout,
                                    VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                    VkAccessFlags srcMask, VkAccessFlags dstMask);

 private:
  bool createInstance();
  bool setupDebugMessenger();
  bool pickPhysicalDevice();
  bool createLogicalDevice();
  bool createCommandPool();

  static bool checkValidationLayerSupport();
  static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
  static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);

 private:
  bool debugOutput_ = false;
  VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;

  QueueFamilyIndices queueIndices_;
  VkQueue graphicsQueue_ = VK_NULL_HANDLE;
  VkCommandPool commandPool_ = VK_NULL_HANDLE;

  VkPhysicalDeviceProperties deviceProperties_{};
  VkPhysicalDeviceMemoryProperties deviceMemoryProperties_{};
};

}
