/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <string>
#include "Base/Logger.h"

#ifdef PLATFORM_OSX
#include "MoltenVK/mvk_vulkan.h"
#else
#include "vulkan/vulkan.h"
#endif

namespace SoftGL {

struct VKContext {
  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;
};

namespace VK {

uint32_t getMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeBits, VkMemoryPropertyFlags properties);

}

#ifdef DEBUG
#define VK_CHECK(stmt) do {                                                                 \
            VkResult result = (stmt);                                                       \
            if (result != VK_SUCCESS) {                                                     \
              LOGE("VK_CHECK: VkResult: %d, %s:%d, %s", result, __FILE__, __LINE__, stmt);  \
              abort();                                                                      \
            }                                                                               \
        } while (0)
#else
#define VK_CHECK(stmt) stmt
#endif

}
