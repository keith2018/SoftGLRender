/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "VKContext.h"
#include "Base/Logger.h"
#include "Base/Timer.h"
#include "VulkanUtils.h"
#include "VKGLInterop.h"

#pragma clang diagnostic push

#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wdeprecated-copy"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wunused-private-field"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wnullability-completeness"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wc++98-compat-extra-semi"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#pragma clang diagnostic pop

namespace SoftGL {

#define COMMAND_BUFFER_POOL_MAX_SIZE 128
#define UNIFORM_BUFFER_POOL_MAX_SIZE 128

#define SEMAPHORE_MAX_SIZE 8

const std::vector<const char *> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char *> kRequiredInstanceExtensions = {
#ifdef PLATFORM_OSX
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
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
  FUNCTION_TIMED("VKContext::create");
#define EXEC_VULKAN_STEP(step)                    \
  if (!step()) {                                  \
    LOGE("initVulkan error: %s failed", #step);   \
    return false;                                 \
  }

  debugOutput_ = debugOutput;

  // query instance extensions
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> instanceExtensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instanceExtensions.data());
  for (auto &ext : instanceExtensions) {
    instanceExtensions_[ext.extensionName] = ext;
  }

  EXEC_VULKAN_STEP(createInstance)
  EXEC_VULKAN_STEP(setupDebugMessenger)
  EXEC_VULKAN_STEP(pickPhysicalDevice)
  EXEC_VULKAN_STEP(createLogicalDevice)
  EXEC_VULKAN_STEP(createCommandPool)

  commandBuffers_.reserve(COMMAND_BUFFER_POOL_MAX_SIZE);

  // vma
  VmaAllocatorCreateInfo allocatorCreateInfo{};
  allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
  allocatorCreateInfo.physicalDevice = physicalDevice_;
  allocatorCreateInfo.device = device_;
  allocatorCreateInfo.instance = instance_;
  allocatorCreateInfo.pVulkanFunctions = nullptr;

  VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &allocator_));

  // OpenGL interop
  VKGLInterop::checkFunctionsAvailable();

  return true;
}

void VKContext::destroy() {
  FUNCTION_TIMED("VKContext::destroy");

  for (auto &kv : uniformBufferPool_) {
    for (auto &buff : kv.second) {
      buff.buffer.destroy(allocator_);
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

  // vma
  vmaDestroyAllocator(allocator_);

  // debug output
  if (debugOutput_) {
    if (VKLoader::vkDestroyDebugUtilsMessengerEXT != nullptr) {
      VKLoader::vkDestroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
    }
  }

  // device & instance
  vkDestroyDevice(device_, nullptr);
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
  if (maxCommandBufferPoolSize_ >= COMMAND_BUFFER_POOL_MAX_SIZE) {
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
      // reset command buffer
      vkFreeCommandBuffers(device_, commandPool_, 1, &cmd.cmdBuffer);
      cmd.cmdBuffer = VK_NULL_HANDLE;

      // reset fence
      vkResetFences(device_, 1, &cmd.fence);

      // reset uniform buffers
      for (auto *buff : cmd.uniformBuffers) {
        buff->inUse = false;
      }
      cmd.uniformBuffers.clear();

      // reset descriptor sets
      for (auto *set : cmd.descriptorSets) {
        set->inUse = false;
      }
      cmd.descriptorSets.clear();

      // reset flag
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

void VKContext::endCommands(CommandBuffer *commandBuffer,
                            const std::vector<VkSemaphore> &waitSemaphores,
                            const std::vector<VkSemaphore> &signalSemaphores) {
  VK_CHECK(vkEndCommandBuffer(commandBuffer->cmdBuffer));

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer->cmdBuffer;

  if (!waitSemaphores.empty()) {
    if (waitSemaphores.size() > SEMAPHORE_MAX_SIZE) {
      LOGE("endCommands error: wait semaphores max size exceeded");
    }
    static std::vector<VkPipelineStageFlags> waitStageMasks(SEMAPHORE_MAX_SIZE, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStageMasks.data();
  } else {
    submitInfo.waitSemaphoreCount = 0;
  }

  if (!signalSemaphores.empty()) {
    submitInfo.signalSemaphoreCount = signalSemaphores.size();
    submitInfo.pSignalSemaphores = signalSemaphores.data();
  } else {
    submitInfo.signalSemaphoreCount = 0;
  }

  VK_CHECK(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, commandBuffer->fence));
  purgeCommandBuffers();
}

void VKContext::waitCommands(CommandBuffer *commandBuffer) {
  VK_CHECK(vkWaitForFences(device_, 1, &commandBuffer->fence, VK_TRUE, UINT64_MAX));
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
    uboPool.reserve(UNIFORM_BUFFER_POOL_MAX_SIZE);
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
  createUniformBuffer(buff.buffer, size);
  buff.mapPtr = buff.buffer.allocInfo.pMappedData;
  pool.push_back(buff);
  maxUniformBufferPoolSize_ = std::max(maxUniformBufferPoolSize_, pool.size());
  if (maxUniformBufferPoolSize_ >= UNIFORM_BUFFER_POOL_MAX_SIZE) {
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
  appInfo.apiVersion = VK_API_VERSION_1_2;

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
  auto &glInteropExt = VKGLInterop::getRequiredInstanceExtensions();
  if (extensionsExits(glInteropExt, instanceExtensions_)) {
    extensions.insert(extensions.end(), glInteropExt.begin(), glInteropExt.end());
  } else {
    LOGE("VKGLInterop required instance extensions not found");
    VKGLInterop::setVkExtensionsAvailable(false);
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

  VKLoader::init(instance_);
  return true;
}

bool VKContext::setupDebugMessenger() {
  if (!debugOutput_) {
    return true;
  }

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);

  VkResult ret;
  if (VKLoader::vkCreateDebugUtilsMessengerEXT != nullptr) {
    ret = VKLoader::vkCreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debugMessenger_);
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

    // query device extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, &extensionCount, deviceExtensions.data());
    for (auto &ext : deviceExtensions) {
      deviceExtensions_[ext.extensionName] = ext;
    }
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
  auto &glInteropExt = VKGLInterop::getRequiredDeviceExtensions();
  if (extensionsExits(glInteropExt, deviceExtensions_)) {
    extensions.insert(extensions.end(), glInteropExt.begin(), glInteropExt.end());
  } else {
    LOGE("VKGLInterop required device extensions not found");
    VKGLInterop::setVkExtensionsAvailable(false);
  }
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

void VKContext::createGPUBuffer(AllocatedBuffer &buffer, VkDeviceSize size, VkBufferUsageFlags usage) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VK_CHECK(vmaCreateBuffer(allocator_, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocInfo));
}

void VKContext::createUniformBuffer(AllocatedBuffer &buffer, VkDeviceSize size) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
  allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  VK_CHECK(vmaCreateBuffer(allocator_, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocInfo));
}

void VKContext::createStagingBuffer(AllocatedBuffer &buffer, VkDeviceSize size) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
  allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

  VK_CHECK(vmaCreateBuffer(allocator_, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocInfo));
}

bool VKContext::createImageMemory(AllocatedImage &image, uint32_t properties, const void *pNext) {
  if (image.memory != VK_NULL_HANDLE) {
    return true;
  }

  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(device_, image.image, &memReqs);
  image.allocationSize = memReqs.size;

  VkMemoryAllocateInfo memAllocInfo{};
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllocInfo.pNext = pNext;
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

bool VKContext::extensionsExits(const std::vector<const char *> &requiredExtensions,
                                const std::unordered_map<std::string, VkExtensionProperties> &availableExtensions) {
  for (auto &reqExt : requiredExtensions) {
    if (availableExtensions.find(reqExt) == availableExtensions.end()) {
      return false;
    }
  }
  return true;
}

}
