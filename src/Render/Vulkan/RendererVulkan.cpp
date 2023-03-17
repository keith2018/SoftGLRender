/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "RendererVulkan.h"
#include "Base/Logger.h"
#include "TextureVulkan.h"
#include "VulkanUtils.h"

namespace SoftGL {

const std::vector<const char *> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char *> kRequiredInstanceExtensions = {
#ifdef PLATFORM_OSX
    "VK_KHR_portability_enumeration",
    "VK_KHR_get_physical_vkCtx_.deviceproperties2",
#endif
};

const std::vector<const char *> kRequiredDeviceExtensions = {
#ifdef PLATFORM_OSX
    "VK_KHR_portability_subset",
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

bool RendererVulkan::create() {
#ifdef DEBUG
  enableValidationLayers_ = true;
#endif
  return initVulkan();
}

void RendererVulkan::destroy() {
  cleanupVulkan();
}

// framebuffer
std::shared_ptr<FrameBuffer> RendererVulkan::createFrameBuffer() {
  return std::make_shared<FrameBufferVulkan>(vkCtx_);
}

// texture
std::shared_ptr<Texture> RendererVulkan::createTexture(const TextureDesc &desc) {
  switch (desc.type) {
    case TextureType_2D:    return std::make_shared<Texture2DVulkan>(vkCtx_, desc);
    case TextureType_CUBE:  return std::make_shared<TextureCubeVulkan>(vkCtx_, desc);
  }
  return nullptr;
}

// vertex
std::shared_ptr<VertexArrayObject> RendererVulkan::createVertexArrayObject(const VertexArray &vertexArray) {
  return nullptr;
}

// shader program
std::shared_ptr<ShaderProgram> RendererVulkan::createShaderProgram() {
  return std::make_shared<ShaderProgramVulkan>(vkCtx_);
}

// uniform
std::shared_ptr<UniformBlock> RendererVulkan::createUniformBlock(const std::string &name, int size) {
  return nullptr;
}

std::shared_ptr<UniformSampler> RendererVulkan::createUniformSampler(const std::string &name, const TextureDesc &desc) {
  return nullptr;
}

// pipeline
void RendererVulkan::setFrameBuffer(std::shared_ptr<FrameBuffer> &frameBuffer) {
  if (!frameBuffer) {
    return;
  }

  fbo_ = dynamic_cast<FrameBufferVulkan *>(frameBuffer.get());
}

void RendererVulkan::setViewPort(int x, int y, int width, int height) {
  viewport_.x = (float) x;
  viewport_.y = (float) y;
  viewport_.width = (float) width;
  viewport_.height = (float) height;
  viewport_.minDepth = 0.0f;
  viewport_.maxDepth = 1.0f;
}

void RendererVulkan::clear(const ClearState &state) {
  if (state.colorFlag) {
    clearValue_.color = {state.clearColor.r, state.clearColor.g, state.clearColor.b, state.clearColor.a};
  }

  if (state.depthFlag) {
    clearValue_.depthStencil.depth = viewport_.maxDepth;
  }
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
  shaderProgram_ = dynamic_cast<ShaderProgramVulkan *>(program.get());
}

void RendererVulkan::setShaderUniforms(std::shared_ptr<ShaderUniforms> &uniforms) {
  if (!uniforms) {
    return;
  }
}

void RendererVulkan::draw(PrimitiveType type) {
  recordDraw(drawCmd_);
  submitWork(drawCmd_, vkCtx_.graphicsQueue);

  recordCopy(copyCmd_);
  submitWork(copyCmd_, vkCtx_.graphicsQueue);
}

bool RendererVulkan::initVulkan() {
#define EXEC_VULKAN_STEP(step)                    \
  if (!step()) {                                  \
    LOGE("initVulkan error: %s failed", #step);   \
    return false;                                 \
  }

  EXEC_VULKAN_STEP(createInstance)
  EXEC_VULKAN_STEP(setupDebugMessenger)
  EXEC_VULKAN_STEP(pickPhysicalDevice)
  EXEC_VULKAN_STEP(createLogicalDevice)
  EXEC_VULKAN_STEP(createRenderPass)
  EXEC_VULKAN_STEP(createGraphicsPipeline)
  EXEC_VULKAN_STEP(createCommandPool)
  EXEC_VULKAN_STEP(createCommandBuffer)

  return true;
}

void RendererVulkan::cleanupVulkan() {
  vkDestroyRenderPass(vkCtx_.device, renderPass_, nullptr);
  vkDestroyPipelineLayout(vkCtx_.device, pipelineLayout_, nullptr);
  vkDestroyPipeline(vkCtx_.device, graphicsPipeline_, nullptr);
  vkDestroyCommandPool(vkCtx_.device, commandPool_, nullptr);

  vkDestroyDevice(vkCtx_.device, nullptr);

  if (enableValidationLayers_) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkCtx_.instance,
                                                                            "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
      func(vkCtx_.instance, debugMessenger_, nullptr);
    }
  }

  vkDestroyInstance(vkCtx_.instance, nullptr);
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
#ifdef PLATFORM_OSX
  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
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

  VK_CHECK(vkCreateInstance(&createInfo, nullptr, &vkCtx_.instance));
  return true;
}

bool RendererVulkan::setupDebugMessenger() {
  if (!enableValidationLayers_) {
    return true;
  }

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);

  VkResult ret = VK_SUCCESS;
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkCtx_.instance,
                                                                         "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    ret = func(vkCtx_.instance, &createInfo, nullptr, &debugMessenger_);
  } else {
    ret = VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  VK_CHECK(ret);
  return true;
}

bool RendererVulkan::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(vkCtx_.instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    LOGE("failed to find GPUs with Vulkan support");
    return false;
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(vkCtx_.instance, &deviceCount, devices.data());

  for (const auto &device : devices) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    if (indices.isComplete()) {
      vkCtx_.physicalDevice = device;
      break;
    }
  }

  return vkCtx_.physicalDevice != VK_NULL_HANDLE;
}

bool RendererVulkan::createLogicalDevice() {
  QueueFamilyIndices indices = findQueueFamilies(vkCtx_.physicalDevice);

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

  VK_CHECK(vkCreateDevice(vkCtx_.physicalDevice, &createInfo, nullptr, &vkCtx_.device));
  vkGetDeviceQueue(vkCtx_.device, indices.graphicsFamily, 0, &vkCtx_.graphicsQueue);
  return true;
}

bool RendererVulkan::createRenderPass() {
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  VK_CHECK(vkCreateRenderPass(vkCtx_.device, &renderPassInfo, nullptr, &renderPass_));
  return true;
}

bool RendererVulkan::createGraphicsPipeline() {
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  std::vector<VkDynamicState> dynamicStates = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR
  };
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pushConstantRangeCount = 0;

  VK_CHECK(vkCreatePipelineLayout(vkCtx_.device, &pipelineLayoutInfo, nullptr, &pipelineLayout_));

  auto &shaderStages = shaderProgram_->getShaderStages();

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = shaderStages.size();
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout_;
  pipelineInfo.renderPass = renderPass_;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  VK_CHECK(vkCreateGraphicsPipelines(vkCtx_.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_));
  return true;
}

bool RendererVulkan::createCommandPool() {
  QueueFamilyIndices queueFamilyIndices = findQueueFamilies(vkCtx_.physicalDevice);

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

  VK_CHECK(vkCreateCommandPool(vkCtx_.device, &poolInfo, nullptr, &commandPool_));
  return true;
}

bool RendererVulkan::createCommandBuffer() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool_;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VK_CHECK(vkAllocateCommandBuffers(vkCtx_.device, &allocInfo, &drawCmd_));
  VK_CHECK(vkAllocateCommandBuffers(vkCtx_.device, &allocInfo, &copyCmd_));
  return true;
}

void RendererVulkan::recordDraw(VkCommandBuffer commandBuffer) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass_;
  renderPassInfo.framebuffer = fbo_->getVKFramebuffer();
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = {fbo_->width(), fbo_->height()};

  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearValue_;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport_);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = {(uint32_t) viewport_.width, (uint32_t) viewport_.height};
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void RendererVulkan::recordCopy(VkCommandBuffer commandBuffer) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  // Transition destination image to transfer destination layout
  VkImageMemoryBarrier imageMemoryBarrier{};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.srcAccessMask = 0;
  imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imageMemoryBarrier.image = fbo_->getHostImage();
  imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &imageMemoryBarrier);

  // colorAttachment.image is already in VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, and does not need to be transitioned

  VkImageCopy imageCopyRegion{};
  imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopyRegion.srcSubresource.layerCount = 1;
  imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopyRegion.dstSubresource.layerCount = 1;
  imageCopyRegion.extent.width = fbo_->width();
  imageCopyRegion.extent.height = fbo_->height();
  imageCopyRegion.extent.depth = 1;

  vkCmdCopyImage(commandBuffer,
                 fbo_->getColorAttachmentImage(),
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 fbo_->getHostImage(),
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 1,
                 &imageCopyRegion);

  // Transition destination image to general layout, which is the required layout for mapping the image memory later on
  imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &imageMemoryBarrier);

  VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void RendererVulkan::submitWork(VkCommandBuffer cmdBuffer, VkQueue queue) {
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuffer;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = 0;

  VkFence fence;
  VK_CHECK(vkCreateFence(vkCtx_.device, &fenceInfo, nullptr, &fence));

  VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, fence));

  VK_CHECK(vkWaitForFences(vkCtx_.device, 1, &fence, VK_TRUE, UINT64_MAX));
  vkDestroyFence(vkCtx_.device, fence, nullptr);
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
