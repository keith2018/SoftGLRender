/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "RendererVulkan.h"
#include "Base/Logger.h"

namespace SoftGL {

const std::vector<const char *> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char *> kRequiredInstanceExtensions = {
    "VK_KHR_portability_enumeration",
    "VK_KHR_get_physical_device_properties2",
};

const std::vector<const char *> kRequiredDeviceExtensions = {
    "VK_KHR_portability_subset",
};

static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                      VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                      void *pUserData) {
  LOGE("validation layer: %s", pCallbackData->pMessage);
  return VK_FALSE;
}

RendererVulkan::RendererVulkan() {
#ifdef DEBUG
  enableValidationLayers_ = true;
#endif
  bool success = initVulkan();
  LOGD("init Vulkan: %s", success ? "success" : "failed");
}

RendererVulkan::~RendererVulkan() {
  cleanupVulkan();
}

// framebuffer
std::shared_ptr<FrameBuffer> RendererVulkan::createFrameBuffer() {
  return nullptr;
}

// texture
std::shared_ptr<Texture> RendererVulkan::createTexture(const TextureDesc &desc) {
  return nullptr;
}

// vertex
std::shared_ptr<VertexArrayObject> RendererVulkan::createVertexArrayObject(const VertexArray &vertexArray) {
  return nullptr;
}

// shader program
std::shared_ptr<ShaderProgram> RendererVulkan::createShaderProgram() {
  return nullptr;
}

// uniform
std::shared_ptr<UniformBlock> RendererVulkan::createUniformBlock(const std::string &name, int size) {
  return nullptr;
}

std::shared_ptr<UniformSampler> RendererVulkan::createUniformSampler(const std::string &name, TextureType type,
                                                                     TextureFormat format) {
  return nullptr;
}

// pipeline
void RendererVulkan::setFrameBuffer(std::shared_ptr<FrameBuffer> &frameBuffer) {
}

void RendererVulkan::setViewPort(int x, int y, int width, int height) {
}

void RendererVulkan::clear(const ClearState &state) {
}

void RendererVulkan::setRenderState(const RenderState &state) {
}

void RendererVulkan::setVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) {
  if (!vao) {
    return;
  }
}

void RendererVulkan::setShaderProgram(std::shared_ptr<ShaderProgram> &program) {
  if (!program) {
    return;
  }
}

void RendererVulkan::setShaderUniforms(std::shared_ptr<ShaderUniforms> &uniforms) {
  if (!uniforms) {
    return;
  }
}

void RendererVulkan::draw(PrimitiveType type) {
}

bool RendererVulkan::initVulkan() {
  if (!createInstance()) {
    LOGE("initVulkan createInstance failed");
    return false;
  }

  if (!setupDebugMessenger()) {
    LOGE("initVulkan setupDebugMessenger failed");
    return false;
  }

  if (!pickPhysicalDevice()) {
    LOGE("initVulkan pickPhysicalDevice failed");
    return false;
  }

  if (!createLogicalDevice()) {
    LOGE("initVulkan createLogicalDevice failed");
    return false;
  }

  return true;
}

void RendererVulkan::cleanupVulkan() {
  vkDestroyDevice(vkDevice_, nullptr);

  if (enableValidationLayers_) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance_,
                                                                            "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
      func(vkInstance_, vkDebugMessenger_, nullptr);
    }
  }

  vkDestroyInstance(vkInstance_, nullptr);
}

bool RendererVulkan::createInstance() {
  if (enableValidationLayers_ && !checkValidationLayerSupport()) {
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
  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = kRequiredInstanceExtensions;
  if (enableValidationLayers_) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (enableValidationLayers_) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
    createInfo.ppEnabledLayerNames = kValidationLayers.data();

    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  VkResult ret = vkCreateInstance(&createInfo, nullptr, &vkInstance_);
  if (ret != VK_SUCCESS) {
    LOGE("vkCreateInstance ret: %d", ret);
    return false;
  }
  return true;
}

bool RendererVulkan::setupDebugMessenger() {
  if (!enableValidationLayers_) {
    return true;
  }

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);

  VkResult ret = VK_SUCCESS;
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance_,
                                                                         "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    ret = func(vkInstance_, &createInfo, nullptr, &vkDebugMessenger_);
  } else {
    ret = VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  if (ret != VK_SUCCESS) {
    LOGE("setupDebugMessenger ret: %d", ret);
    return false;
  }
  return true;
}

bool RendererVulkan::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, nullptr);

  if (deviceCount == 0) {
    LOGE("failed to find GPUs with Vulkan support");
    return false;
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, devices.data());

  for (const auto &device : devices) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    if (indices.isComplete()) {
      vkPhysicalDevice_ = device;
      break;
    }
  }

  return vkPhysicalDevice_ != VK_NULL_HANDLE;
}

bool RendererVulkan::createLogicalDevice() {
  QueueFamilyIndices indices = findQueueFamilies(vkPhysicalDevice_);

  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
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

  if (enableValidationLayers_) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
    createInfo.ppEnabledLayerNames = kValidationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  VkResult ret = vkCreateDevice(vkPhysicalDevice_, &createInfo, nullptr, &vkDevice_);
  if (ret != VK_SUCCESS) {
    LOGE("vkCreateDevice ret: %d", ret);
    return false;
  }

  vkGetDeviceQueue(vkDevice_, indices.graphicsFamily, 0, &vkGraphicsQueue_);
  return true;
}

bool RendererVulkan::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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

void RendererVulkan::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
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

QueueFamilyIndices RendererVulkan::findQueueFamilies(VkPhysicalDevice device) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

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

}
