/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <glad/glad.h>
#include "Render/Vulkan/VKContext.h"

namespace SoftGL {

#ifdef PLATFORM_WINDOWS
#  define glImportSemaphore glImportSemaphoreWin32HandleEXT
#  define glImportMemory glImportMemoryWin32HandleEXT
#  define GL_HANDLE_TYPE GL_HANDLE_TYPE_OPAQUE_WIN32_EXT

#  define VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT
#  define VK_EXTERNAL_MEMORY_HANDLE_TYPE VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
#  define VK_STRUCTURE_TYPE_SEMAPHORE_GET_HANDLE_INFO VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR
#  define VK_STRUCTURE_TYPE_MEMORY_GET_HANDLE_INFO VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR
#  define VkSemaphoreGetHandleInfo VkSemaphoreGetWin32HandleInfoKHR
#  define VkMemoryGetHandleInfo VkMemoryGetWin32HandleInfoKHR
#  define vkGetSemaphoreHandle VKLoader::vkGetSemaphoreWin32HandleKHR
#  define vkGetMemoryHandle VKLoader::vkGetMemoryWin32HandleKHR
#else
#  define glImportSemaphore glImportSemaphoreFdEXT
#  define glImportMemory glImportMemoryFdEXT
#  define GL_HANDLE_TYPE GL_HANDLE_TYPE_OPAQUE_FD_EXT

#  define VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT
#  define VK_EXTERNAL_MEMORY_HANDLE_TYPE VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
#  define VK_STRUCTURE_TYPE_SEMAPHORE_GET_HANDLE_INFO VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR
#  define VK_STRUCTURE_TYPE_MEMORY_GET_HANDLE_INFO VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR
#  define VkSemaphoreGetHandleInfo VkSemaphoreGetFdInfoKHR
#  define VkMemoryGetHandleInfo VkMemoryGetFdInfoKHR
#  define vkGetSemaphoreHandle VKLoader::vkGetSemaphoreFdKHR
#  define vkGetMemoryHandle VKLoader::vkGetMemoryFdKHR
#endif

#ifdef PLATFORM_WINDOWS
using Handle = HANDLE;
#else
using Handle                          = int;
constexpr Handle INVALID_HANDLE_VALUE = static_cast<Handle>(-1);
#endif

struct SharedSemaphore {
  Handle handle = INVALID_HANDLE_VALUE;
  VkSemaphore vkRef = VK_NULL_HANDLE;
  GLuint glRef = 0;
};

struct SharedMemory {
  Handle handle = INVALID_HANDLE_VALUE;
  VkDeviceSize allocationSize = 0;
  VkDeviceMemory vkRef = VK_NULL_HANDLE;
  GLuint glRef = 0;
  GLuint glAttachedTexture = 0;
};

class VKGLInterop {
 public:
  explicit VKGLInterop(VKContext &ctx) : vkCtx_(ctx) {
    device_ = ctx.device();
  }
  ~VKGLInterop();

  static inline const void *getExtImageCreateInfo() { return &extMemoryImageCreateInfo_; }
  static inline const void *getExtMemoryAllocateInfo() { return &exportMemoryAllocateInfo_; }

  inline VkSemaphore getSemaphoreGLReady() const { return glReady_.vkRef; }
  inline VkSemaphore getSemaphoreGLComplete() const { return glComplete_.vkRef; }

  void createSharedSemaphores();
  void createSharedMemory(VkDeviceMemory memory, VkDeviceSize allocationSize);

  void setGLTexture2DStorage(GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height);

  void waitGLReady();
  void signalGLComplete();

 public:
  static void checkFunctionsAvailable();
  static inline void setVkExtensionsAvailable(bool available) { vkExtensionsAvailable_ = available; };
  static inline bool isAvailable() { return vkExtensionsAvailable_ && functionsAvailable_; }

  static inline const std::vector<const char *> &getRequiredInstanceExtensions() { return requiredInstanceExtensions; }
  static inline const std::vector<const char *> &getRequiredDeviceExtensions() { return requiredDeviceExtensions; }

 private:
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  SharedMemory sharedMemory_{};
  SharedSemaphore glReady_{};
  SharedSemaphore glComplete_{};

 private:
  static bool vkExtensionsAvailable_;
  static bool functionsAvailable_;
  static const std::vector<const char *> requiredInstanceExtensions;
  static const std::vector<const char *> requiredDeviceExtensions;

  static VkExternalMemoryImageCreateInfo extMemoryImageCreateInfo_;
  static VkExportMemoryAllocateInfo exportMemoryAllocateInfo_;
};

}
