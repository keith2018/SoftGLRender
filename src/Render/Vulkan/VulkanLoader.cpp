/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "VulkanLoader.h"

namespace SoftGL {

INIT_VK_FUNC(vkCreateDebugUtilsMessengerEXT);
INIT_VK_FUNC(vkDestroyDebugUtilsMessengerEXT);

#ifdef PLATFORM_WINDOWS
INIT_VK_FUNC(vkGetSemaphoreWin32HandleKHR);
INIT_VK_FUNC(vkGetMemoryWin32HandleKHR);
#else
INIT_VK_FUNC(vkGetSemaphoreFdKHR);
INIT_VK_FUNC(vkGetMemoryFdKHR);
#endif

}
