/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Render/Texture.h"
#include "Render/PipelineStates.h"
#include "VulkanUtils.h"

namespace SoftGL {
namespace VK {

static inline VkImageType cvtImageType(TextureType type) {
  switch (type) {
    case TextureType_2D:
    case TextureType_CUBE:      return VK_IMAGE_TYPE_2D;
  }
  return VK_IMAGE_TYPE_MAX_ENUM;
}

static inline VkFormat cvtImageFormat(TextureFormat format) {
  switch (format) {
    case TextureFormat_RGBA8:   return VK_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat_FLOAT32: return VK_FORMAT_R32_SFLOAT;
  }
  return VK_FORMAT_MAX_ENUM;
}

static inline VkImageViewType cvtImageViewType(TextureType type) {
  switch (type) {
    case TextureType_2D:        return VK_IMAGE_VIEW_TYPE_2D;
    case TextureType_CUBE:      return VK_IMAGE_VIEW_TYPE_CUBE;
  }
  return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
}

static inline VkPrimitiveTopology cvtPrimitiveTopology(PrimitiveType type) {
  switch (type) {
    case Primitive_POINT:       return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case Primitive_LINE:        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case Primitive_TRIANGLE:    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  }
  return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
}

static inline VkPolygonMode cvtPolygonMode(PolygonMode mode) {
  switch (mode) {
    case PolygonMode_POINT:     return VK_POLYGON_MODE_POINT;
    case PolygonMode_LINE:      return VK_POLYGON_MODE_LINE;
    case PolygonMode_FILL:      return VK_POLYGON_MODE_FILL;
  }
  return VK_POLYGON_MODE_MAX_ENUM;
}

}
}
