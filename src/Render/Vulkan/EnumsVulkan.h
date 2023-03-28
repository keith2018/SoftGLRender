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
    case TextureType_CUBE:          return VK_IMAGE_TYPE_2D;
  }
  return VK_IMAGE_TYPE_MAX_ENUM;
}

static inline VkFormat cvtImageFormat(TextureFormat format, uint32_t usage) {
  if (usage & TextureUsage_AttachmentDepth) {
    switch (format) {
      case TextureFormat_FLOAT32: return VK_FORMAT_D32_SFLOAT;
      default:
        break;
    }
  } else {
    switch (format) {
      case TextureFormat_RGBA8:   return VK_FORMAT_R8G8B8A8_UNORM;
      case TextureFormat_FLOAT32: return VK_FORMAT_R32_SFLOAT;
      default:
        break;
    }
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

static inline VkCompareOp cvtDepthFunc(DepthFunction func) {
  switch (func) {
    case DepthFunc_NEVER:       return VK_COMPARE_OP_NEVER;
    case DepthFunc_LESS:        return VK_COMPARE_OP_LESS ;
    case DepthFunc_EQUAL:       return VK_COMPARE_OP_EQUAL ;
    case DepthFunc_LEQUAL:      return VK_COMPARE_OP_LESS_OR_EQUAL ;
    case DepthFunc_GREATER:     return VK_COMPARE_OP_GREATER ;
    case DepthFunc_NOTEQUAL:    return VK_COMPARE_OP_NOT_EQUAL ;
    case DepthFunc_GEQUAL:      return VK_COMPARE_OP_GREATER_OR_EQUAL ;
    case DepthFunc_ALWAYS:      return VK_COMPARE_OP_ALWAYS ;
    default:
      break;
  }
  return VK_COMPARE_OP_MAX_ENUM;
}

static inline VkBlendFactor cvtBlendFactor(BlendFactor factor) {
  switch (factor) {
    case BlendFactor_ZERO:                return VK_BLEND_FACTOR_ZERO;
    case BlendFactor_ONE:                 return VK_BLEND_FACTOR_ONE;
    case BlendFactor_SRC_COLOR:           return VK_BLEND_FACTOR_SRC_COLOR;
    case BlendFactor_SRC_ALPHA:           return VK_BLEND_FACTOR_SRC_ALPHA;
    case BlendFactor_DST_COLOR:           return VK_BLEND_FACTOR_DST_COLOR;
    case BlendFactor_DST_ALPHA:           return VK_BLEND_FACTOR_DST_ALPHA;
    case BlendFactor_ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case BlendFactor_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case BlendFactor_ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case BlendFactor_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    default:
      break;
  }
  return VK_BLEND_FACTOR_MAX_ENUM;
}

static inline VkBlendOp cvtBlendFunction(BlendFunction func) {
  switch (func) {
    case BlendFunc_ADD:                   return VK_BLEND_OP_ADD;
    case BlendFunc_SUBTRACT:              return VK_BLEND_OP_SUBTRACT;
    case BlendFunc_REVERSE_SUBTRACT:      return VK_BLEND_OP_REVERSE_SUBTRACT;
    case BlendFunc_MIN:                   return VK_BLEND_OP_MIN;
    case BlendFunc_MAX:                   return VK_BLEND_OP_MAX;
    default:
      break;
  }
  return VK_BLEND_OP_MAX_ENUM;
}

}
}
