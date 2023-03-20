/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/Platform.h"

#ifdef PLATFORM_OSX
#include "MoltenVK/mvk_vulkan.h"
#else
#include "vulkan/vulkan.h"
#endif
