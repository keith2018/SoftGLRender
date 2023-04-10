/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "VKGLInterop.h"
#include "Base/Logger.h"
#include "Render/OpenGL/OpenGLUtils.h"
#include "Render/Vulkan/VulkanUtils.h"

namespace SoftGL {

#ifdef PLATFORM_WINDOWS
constexpr const char *HOST_MEMORY_EXTENSION_NAME = VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME;
constexpr const char *HOST_SEMAPHORE_EXTENSION_NAME = VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME;
#else
constexpr const char *HOST_MEMORY_EXTENSION_NAME = VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME;
constexpr const char *HOST_SEMAPHORE_EXTENSION_NAME = VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME;
#endif

bool VKGLInterop::vkExtensionsAvailable_ = true;
bool VKGLInterop::functionsAvailable_ = false;
VkExternalMemoryImageCreateInfo VKGLInterop::extMemoryImageCreateInfo_{};
VkExportMemoryAllocateInfo VKGLInterop::exportMemoryAllocateInfo_{};

const std::vector<const char *> VKGLInterop::requiredInstanceExtensions = {
    VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
    VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
};

const std::vector<const char *> VKGLInterop::requiredDeviceExtensions = {
    VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
    VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
    HOST_SEMAPHORE_EXTENSION_NAME,
    HOST_MEMORY_EXTENSION_NAME,
};

VKGLInterop::~VKGLInterop() {
  if (!isAvailable()) {
    return;
  }

  vkDestroySemaphore(device_, glReady_.vkRef, nullptr);
  GL_CHECK(glDeleteSemaphoresEXT(1, &glReady_.glRef));

  vkDestroySemaphore(device_, glComplete_.vkRef, nullptr);
  GL_CHECK(glDeleteSemaphoresEXT(1, &glComplete_.glRef));

  GL_CHECK(glDeleteMemoryObjectsEXT(1, &sharedMemory_.glRef));
}

void VKGLInterop::checkFunctionsAvailable() {
  // check gl extensions
  if (!glGenSemaphoresEXT || !glImportSemaphore || !glDeleteSemaphoresEXT
      || !glWaitSemaphoreEXT || !glSignalSemaphoreEXT) {
    LOGE("VKGLInterop::checkFunctionsAvailable failed: gl extern semaphore functions not found");
    return;
  }

  if (!glCreateMemoryObjectsEXT || !glImportMemory || !glDeleteMemoryObjectsEXT
      || !glTextureStorageMem2DEXT) {
    LOGE("VKGLInterop::checkFunctionsAvailable failed: gl extern memory functions not found");
    return;
  }

  // check vulkan extensions
  if (!vkGetSemaphoreHandle || !vkGetMemoryHandle) {
    LOGE("VKGLInterop::checkFunctionsAvailable failed: vulkan extension functions not found");
    return;
  }

  // init structures
  extMemoryImageCreateInfo_.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
  extMemoryImageCreateInfo_.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE;

  exportMemoryAllocateInfo_.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
  exportMemoryAllocateInfo_.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE;

  functionsAvailable_ = true;
}

void VKGLInterop::createSharedSemaphores() {
  // create vulkan object
  VkExportSemaphoreCreateInfo exportSemaphoreCreateInfo{};
  exportSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
  exportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE;

  VkSemaphoreCreateInfo semaphoreCreateInfo{};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphoreCreateInfo.pNext = &exportSemaphoreCreateInfo;

  VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &glReady_.vkRef));
  VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &glComplete_.vkRef));

  VkSemaphoreGetHandleInfo semaphoreGetInfo{};
  semaphoreGetInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_HANDLE_INFO;
  semaphoreGetInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE;

  semaphoreGetInfo.semaphore = glReady_.vkRef;
  VK_CHECK(vkGetSemaphoreHandle(device_, &semaphoreGetInfo, &glReady_.handle));
  semaphoreGetInfo.semaphore = glComplete_.vkRef;
  VK_CHECK(vkGetSemaphoreHandle(device_, &semaphoreGetInfo, &glComplete_.handle));

  // create opengl object
  GL_CHECK(glGenSemaphoresEXT(1, &glReady_.glRef));
  GL_CHECK(glImportSemaphore(glReady_.glRef, GL_HANDLE_TYPE, glReady_.handle));

  GL_CHECK(glGenSemaphoresEXT(1, &glComplete_.glRef));
  GL_CHECK(glImportSemaphore(glComplete_.glRef, GL_HANDLE_TYPE, glComplete_.handle));
}

void VKGLInterop::createSharedMemory(VkDeviceMemory memory, VkDeviceSize allocationSize) {
  VkMemoryGetHandleInfo memoryGetInfo{};
  memoryGetInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_HANDLE_INFO;
  memoryGetInfo.memory = memory;
  memoryGetInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE;
  VK_CHECK(vkGetMemoryHandle(device_, &memoryGetInfo, &sharedMemory_.handle));

  sharedMemory_.vkRef = memory;
  sharedMemory_.allocationSize = allocationSize;

  // create opengl object
  GL_CHECK(glCreateMemoryObjectsEXT(1, &sharedMemory_.glRef));
  GL_CHECK(glImportMemory(sharedMemory_.glRef, sharedMemory_.allocationSize, GL_HANDLE_TYPE, sharedMemory_.handle));
}

void VKGLInterop::setGLTexture2DStorage(GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height) {
  sharedMemory_.glAttachedTexture = texture;
  GL_CHECK(glTextureStorageMem2DEXT(texture, levels, internalFormat, width, height, sharedMemory_.glRef, 0));
}

void VKGLInterop::waitGLReady() {
  GLenum srcLayout = GL_LAYOUT_COLOR_ATTACHMENT_EXT;
  GL_CHECK(glWaitSemaphoreEXT(glReady_.glRef, 0, nullptr, 1, &sharedMemory_.glAttachedTexture, &srcLayout));
}

void VKGLInterop::signalGLComplete() {
  GLenum dstLayout = GL_LAYOUT_SHADER_READ_ONLY_EXT;
  GL_CHECK(glSignalSemaphoreEXT(glComplete_.glRef, 0, nullptr, 1, &sharedMemory_.glAttachedTexture, &dstLayout));
  GL_CHECK(glFlush());
}

}
