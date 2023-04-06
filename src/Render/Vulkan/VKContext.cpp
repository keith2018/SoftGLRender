/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "VKContext.h"
#include "Base/Logger.h"
#include "VulkanUtils.h"

namespace SoftGL {

#define COMMAND_BUFFER_POOL_RESERVE 32
#define UNIFORM_BUFFER_POOL_RESERVE 128

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
#ifndef VULKAN_LOG_INFO
      return VK_FALSE;
#else
      break;
#endif
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

  commandBuffers_.reserve(COMMAND_BUFFER_POOL_RESERVE);

  return true;
}

void VKContext::destroy() {
  for (auto &kv : uniformBufferPool_) {
    for (auto &buff : kv.second) {
      vkUnmapMemory(device_, buff.buffer.memory);
      buff.buffer.destroy(device_);
      buff.mapPtr = nullptr;
      buff.inUse = false;
    }
  }

  for (auto &cmd : commandBuffers_) {
    vkDestroyFence(device_, cmd.fence, nullptr);
    vkDestroySemaphore(device_, cmd.semaphore, nullptr);
    if (cmd.cmdBuffer != VK_NULL_HANDLE) {
      vkFreeCommandBuffers(device_, commandPool_, 1, &cmd.cmdBuffer);
    }
  }

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

CommandBuffer *VKContext::getNewCommandBuffer() {
  for (auto &cmd : commandBuffers_) {
    if (!cmd.inUse) {
      if (cmd.cmdBuffer == VK_NULL_HANDLE) {
        allocateCommandBuffer(cmd.cmdBuffer);
      }
      cmd.inUse = true;
      return &cmd;
    }
  }

  CommandBuffer cmd{};
  cmd.inUse = true;
  allocateCommandBuffer(cmd.cmdBuffer);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &cmd.semaphore);

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  VK_CHECK(vkCreateFence(device_, &fenceInfo, nullptr, &cmd.fence));

  commandBuffers_.push_back(cmd);
  maxCommandBufferPoolSize_ = std::max(maxCommandBufferPoolSize_, commandBuffers_.size());
  if (maxCommandBufferPoolSize_ > COMMAND_BUFFER_POOL_RESERVE) {
    LOGE("error: command buffer pool size exceed");
  }
  return &commandBuffers_.back();
}

void VKContext::purgeCommandBuffers() {
  for (auto &cmd : commandBuffers_) {
    if (!cmd.inUse) {
      continue;
    }
    VkResult waitRet = vkGetFenceStatus(device_, cmd.fence);
    if (waitRet == VK_SUCCESS) {
      vkResetFences(device_, 1, &cmd.fence);
      vkFreeCommandBuffers(device_, commandPool_, 1, &cmd.cmdBuffer);
      cmd.cmdBuffer = VK_NULL_HANDLE;
      for (auto &buff : cmd.uniformBuffers) {
        buff->inUse = false;
      }
      cmd.uniformBuffers.clear();
      cmd.inUse = false;
    }
  }
}

CommandBuffer *VKContext::beginCommands() {
  auto *commandBuffer = getNewCommandBuffer();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VK_CHECK(vkBeginCommandBuffer(commandBuffer->cmdBuffer, &beginInfo));

  return commandBuffer;
}

void VKContext::endCommands(CommandBuffer *commandBuffer, VkSemaphore semaphore, bool waitOnHost) {
  VK_CHECK(vkEndCommandBuffer(commandBuffer->cmdBuffer));

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer->cmdBuffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &commandBuffer->semaphore;
  if (semaphore != VK_NULL_HANDLE) {
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphore;
    submitInfo.pWaitDstStageMask = &waitStageMask;
  }
  VK_CHECK(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, commandBuffer->fence));

  if (waitOnHost) {
    VK_CHECK(vkWaitForFences(device_, 1, &commandBuffer->fence, VK_TRUE, UINT64_MAX));
  }

  purgeCommandBuffers();
}

void VKContext::allocateCommandBuffer(VkCommandBuffer &cmdBuffer) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool_;
  allocInfo.commandBufferCount = 1;

  VK_CHECK(vkAllocateCommandBuffers(device_, &allocInfo, &cmdBuffer));
}

UniformBuffer *VKContext::getNewUniformBuffer(VkDeviceSize size) {
  auto it = uniformBufferPool_.find(size);
  if (it == uniformBufferPool_.end()) {
    std::vector<UniformBuffer> uboPool;
    uboPool.reserve(UNIFORM_BUFFER_POOL_RESERVE);
    uniformBufferPool_[size] = std::move(uboPool);
  }
  auto &pool = uniformBufferPool_[size];

  for (auto &buff : pool) {
    if (!buff.inUse) {
      buff.inUse = true;
      return &buff;
    }
  }

  UniformBuffer buff{};
  buff.inUse = true;
  createBuffer(buff.buffer, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  VK_CHECK(vkMapMemory(device_, buff.buffer.memory, 0, size, 0, &buff.mapPtr));
  pool.push_back(buff);
  maxUniformBufferPoolSize_ = std::max(maxUniformBufferPoolSize_, pool.size());
  if (maxUniformBufferPoolSize_ > UNIFORM_BUFFER_POOL_RESERVE) {
    LOGE("error: uniform buffer pool size exceed");
  }
  return &pool.back();
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

  if (ret != VK_SUCCESS) {
    LOGE("VkResult: %s, %s:%d, %s", vkResultStr(ret), __FILE__, __LINE__, "vkCreateDebugUtilsMessengerEXT");
    return false;
  }
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

void VKContext::createBuffer(AllocatedBuffer &buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK(vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer.buffer));
  buffer.size = size;

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device_, buffer.buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = getMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

  VK_CHECK(vkAllocateMemory(device_, &allocInfo, nullptr, &buffer.memory));
  VK_CHECK(vkBindBufferMemory(device_, buffer.buffer, buffer.memory, 0));
}

void VKContext::createStagingBuffer(AllocatedBuffer &buffer, VkDeviceSize size) {
  createBuffer(buffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

bool VKContext::createImageMemory(AllocatedImage &image, uint32_t properties) {
  if (image.memory != VK_NULL_HANDLE) {
    return true;
  }

  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(device_, image.image, &memReqs);

  VkMemoryAllocateInfo memAllocInfo{};
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllocInfo.allocationSize = memReqs.size;
  memAllocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, properties);
  if (memAllocInfo.memoryTypeIndex == VK_MAX_MEMORY_TYPES) {
    LOGE("vulkan memory type not available, property flags: 0x%x", properties);
    return false;
  }
  VK_CHECK(vkAllocateMemory(device_, &memAllocInfo, nullptr, &image.memory));
  VK_CHECK(vkBindImageMemory(device_, image.image, image.memory, 0));

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

}
