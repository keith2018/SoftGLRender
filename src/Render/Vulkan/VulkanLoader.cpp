/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "VulkanLoader.h"

namespace SoftGL {

INIT_VK_FUNC(vkCreateDebugUtilsMessengerEXT);
INIT_VK_FUNC(vkDestroyDebugUtilsMessengerEXT);

INIT_VK_FUNC(vkGetSemaphoreWin32HandleKHR);
INIT_VK_FUNC(vkGetSemaphoreFdKHR);
INIT_VK_FUNC(vkGetMemoryWin32HandleKHR);
INIT_VK_FUNC(vkGetMemoryFdKHR);

}
