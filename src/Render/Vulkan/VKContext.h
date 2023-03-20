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

  inline VkDevice device() const {
    return device_;
  }

  inline VkQueue getGraphicsQueue() const {
    return graphicsQueue_;
  }

  inline VkCommandPool getCommandPool() const {
    return commandPool_;
  }

  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);

  uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);

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

  VkPhysicalDeviceMemoryProperties deviceMemoryProperties_{};
};

}
