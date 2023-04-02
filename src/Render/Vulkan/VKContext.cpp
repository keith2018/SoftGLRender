/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "VKContext.h"
#include "Base/Logger.h"
#include "VulkanUtils.h"

namespace SoftGL {

const std::vector<const char *> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char *> kRequiredInstanceExtensions = {
#ifdef PLATFORM_OSX
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#endif
};

const std::vector<const char *> kRequiredDeviceExtensions = {
#ifdef PLATFORM_OSX
    VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
#endif
};

static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                      VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                      void *pUserData) {
  LogLevel level = LOG_INFO;
  switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      level = LOG_INFO;
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      level = LOG_WARNING;
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      level = LOG_ERROR;
      break;
    default:
      break;
  }
  SoftGL::Logger::log(level, __FILE__, __LINE__, "validation layer: %s", pCallbackData->pMessage);
  return VK_FALSE;
}

bool VKContext::create(bool debugOutput) {
#define EXEC_VULKAN_STEP(step)                    \
  if (!step()) {                                  \
    LOGE("initVulkan error: %s failed", #step);   \
    return false;                                 \
  }

  debugOutput_ = debugOutput;

  EXEC_VULKAN_STEP(createInstance)
  EXEC_VULKAN_STEP(setupDebugMessenger)
  EXEC_VULKAN_STEP(pickPhysicalDevice)
  EXEC_VULKAN_STEP(createLogicalDevice)
  EXEC_VULKAN_STEP(createCommandPool)

  return true;
}

void VKContext::destroy() {
  vkDestroyCommandPool(device_, commandPool_, nullptr);
  vkDestroyDevice(device_, nullptr);

  if (debugOutput_) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance_,
                                                                            "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
      func(instance_, debugMessenger_, nullptr);
    }
  }

  vkDestroyInstance(instance_, nullptr);
}

VkCommandBuffer VKContext::beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool_;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  VK_CHECK(vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer));

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  return commandBuffer;
}

void VKContext::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  VK_CHECK(vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  VkFence fence;
  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  VK_CHECK(vkCreateFence(device_, &fenceInfo, nullptr, &fence));

  VK_CHECK(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, fence));

  VK_CHECK(vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX));
  vkDestroyFence(device_, fence, nullptr);
  vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

uint32_t VKContext::getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) {
  for (uint32_t i = 0; i < deviceMemoryProperties_.memoryTypeCount; i++) {
    if ((typeBits & 1) == 1) {
      if ((deviceMemoryProperties_.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
      }
    }
    typeBits >>= 1;
  }
  return VK_MAX_MEMORY_TYPES;
}

bool VKContext::createInstance() {
  if (debugOutput_ && !checkValidationLayerSupport()) {
    LOGE("validation layers requested, but not available!");
    return false;
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "SoftGLRender";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "VulkanRenderer";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef PLATFORM_OSX
  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = kRequiredInstanceExtensions;
  if (debugOutput_) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (debugOutput_) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
    createInfo.ppEnabledLayerNames = kValidationLayers.data();

    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance_));
  return true;
}

bool VKContext::setupDebugMessenger() {
  if (!debugOutput_) {
    return true;
  }

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);

  VkResult ret;
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance_,
                                                                         "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    ret = func(instance_, &createInfo, nullptr, &debugMessenger_);
  } else {
    ret = VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  VK_CHECK(ret);
  return true;
}

bool VKContext::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  VK_CHECK(vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr));

  if (deviceCount == 0) {
    LOGE("failed to find GPUs with Vulkan support");
    return false;
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  VK_CHECK(vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data()));

  for (const auto &device : devices) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    if (indices.isComplete()) {
      queueIndices_ = indices;
      physicalDevice_ = device;
      break;
    }
  }

  if (physicalDevice_ != VK_NULL_HANDLE) {
    vkGetPhysicalDeviceProperties(physicalDevice_, &deviceProperties_);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &deviceMemoryProperties_);
  }

  return physicalDevice_ != VK_NULL_HANDLE;
}

bool VKContext::createLogicalDevice() {
  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = queueIndices_.graphicsFamily;
  queueCreateInfo.queueCount = 1;

  float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pEnabledFeatures = &deviceFeatures;

  auto extensions = kRequiredDeviceExtensions;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (debugOutput_) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
    createInfo.ppEnabledLayerNames = kValidationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  VK_CHECK(vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_));
  vkGetDeviceQueue(device_, queueIndices_.graphicsFamily, 0, &graphicsQueue_);
  return true;
}

bool VKContext::createCommandPool() {
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueIndices_.graphicsFamily;

  VK_CHECK(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_));
  return true;
}

bool VKContext::checkValidationLayerSupport() {
  uint32_t layerCount;
  VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

  std::vector<VkLayerProperties> availableLayers(layerCount);
  VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

  for (const char *layerName : kValidationLayers) {
    bool layerFound = false;
    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound) {
      return false;
    }
  }
  return true;
}

void VKContext::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = vkDebugCallback;
}

QueueFamilyIndices VKContext::findQueueFamilies(VkPhysicalDevice physicalDevice) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

  QueueFamilyIndices indices;
  for (int i = 0; i < queueFamilies.size(); i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }
    if (indices.isComplete()) {
      break;
    }
  }

  return indices;
}

void VKContext::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags imageAspect, uint32_t mipLevels,
                                      VkImageLayout oldLayout, VkImageLayout newLayout,
                                      VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                      VkAccessFlags srcMask, VkAccessFlags dstMask) {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcAccessMask = srcMask;
  barrier.dstAccessMask = dstMask;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = imageAspect;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VKContext::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                             VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK(vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer));

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = getMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

  VK_CHECK(vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory));
  VK_CHECK(vkBindBufferMemory(device_, buffer, bufferMemory, 0));
}

void VKContext::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}

void VKContext::uploadBufferData(VkBuffer &buffer, void *bufferData, VkDeviceSize bufferSize) {
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);

  void *dataPtr = nullptr;
  VK_CHECK(vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &dataPtr));
  memcpy(dataPtr, bufferData, (size_t) bufferSize);
  vkUnmapMemory(device_, stagingBufferMemory);

  copyBuffer(stagingBuffer, buffer, bufferSize);

  vkDestroyBuffer(device_, stagingBuffer, nullptr);
  vkFreeMemory(device_, stagingBufferMemory, nullptr);
}

bool VKContext::createImageMemory(VkDeviceMemory &memory, VkImage &image, uint32_t properties) {
  if (memory != VK_NULL_HANDLE) {
    return true;
  }

  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(device_, image, &memReqs);

  VkMemoryAllocateInfo memAllocInfo{};
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllocInfo.allocationSize = memReqs.size;
  memAllocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, properties);
  if (memAllocInfo.memoryTypeIndex == VK_MAX_MEMORY_TYPES) {
    LOGE("vulkan memory type not available, property flags: 0x%x", properties);
    return false;
  }
  VK_CHECK(vkAllocateMemory(device_, &memAllocInfo, nullptr, &memory));
  VK_CHECK(vkBindImageMemory(device_, image, memory, 0));

  return true;
}

bool VKContext::linearBlitAvailable(VkFormat format) {
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &formatProperties);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    return false;
  }

  return true;
}

void VKContext::submitWork(VkCommandBuffer cmdBuffer, VkFence fence) {
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuffer;
  VK_CHECK(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, fence));
}

}
