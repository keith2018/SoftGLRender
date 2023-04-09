/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/Platform.h"

#ifdef PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifdef PLATFORM_OSX
#include "MoltenVK/mvk_vulkan.h"
#else
#include "vulkan/vulkan.h"
#endif

#define REG_VK_FUNC(name) static PFN_##name name
#define LOAD_VK_FUNC(name) name = (PFN_##name) vkGetInstanceProcAddr(instance, #name)
#define INIT_VK_FUNC(name) PFN_##name VKLoader::name = nullptr

namespace SoftGL {

class VKLoader {
 public:
  static void init(VkInstance instance) {
    LOAD_VK_FUNC(vkCreateDebugUtilsMessengerEXT);
    LOAD_VK_FUNC(vkDestroyDebugUtilsMessengerEXT);

    LOAD_VK_FUNC(vkGetSemaphoreWin32HandleKHR);
    LOAD_VK_FUNC(vkGetSemaphoreFdKHR);
    LOAD_VK_FUNC(vkGetMemoryWin32HandleKHR);
    LOAD_VK_FUNC(vkGetMemoryFdKHR);
  }

 public:
  REG_VK_FUNC(vkCreateDebugUtilsMessengerEXT);
  REG_VK_FUNC(vkDestroyDebugUtilsMessengerEXT);

  REG_VK_FUNC(vkGetSemaphoreWin32HandleKHR);
  REG_VK_FUNC(vkGetSemaphoreFdKHR);
  REG_VK_FUNC(vkGetMemoryWin32HandleKHR);
  REG_VK_FUNC(vkGetMemoryFdKHR);
};

}
