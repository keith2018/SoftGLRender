/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Render/Texture.h"
#include "Render/RenderState.h"
#include "VulkanUtils.h"

namespace SoftGL {
namespace VK {

static inline VkImageType cvtImageType(TextureType type) {
  switch (type) {
    case TextureType_2D:
    case TextureType_CUBE:
      return VK_IMAGE_TYPE_2D;
  }
}

static inline VkFormat cvtImageFormat(TextureFormat format) {
  switch (format) {
    case TextureFormat_RGBA8:
      return VK_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat_FLOAT32:
      return VK_FORMAT_R32_SFLOAT;
  }
}

static inline VkImageViewType cvtImageViewType(TextureType type) {
  switch (type) {
    case TextureType_2D:
      return VK_IMAGE_VIEW_TYPE_2D;
    case TextureType_CUBE:
      return VK_IMAGE_VIEW_TYPE_CUBE;
  }
}

}
}
