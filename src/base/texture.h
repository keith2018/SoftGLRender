/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <utility>
#include <functional>
#include <atomic>
#include <thread>

#include "frame_buffer.h"

namespace SoftGL {

#define Vec4 glm::tvec4

struct EnumClassHash {
  template<typename T>
  std::size_t operator()(T t) const {
    return static_cast<std::size_t>(t);
  }
};

enum TextureType {
  TextureType_NONE = 0,
  TextureType_DIFFUSE,
  TextureType_NORMALS,
  TextureType_EMISSIVE,
  TextureType_PBR_ALBEDO,
  TextureType_PBR_METAL_ROUGHNESS,
  TextureType_PBR_AMBIENT_OCCLUSION,

  TextureType_CUBE_MAP_POSITIVE_X,
  TextureType_CUBE_MAP_NEGATIVE_X,
  TextureType_CUBE_MAP_POSITIVE_Y,
  TextureType_CUBE_MAP_NEGATIVE_Y,
  TextureType_CUBE_MAP_POSITIVE_Z,
  TextureType_CUBE_MAP_NEGATIVE_Z,

  TextureType_CUBE,
  TextureType_EQUIRECTANGULAR,

  TextureType_IBL_IRRADIANCE,
  TextureType_IBL_PREFILTER,
};

enum WrapMode {
  Wrap_REPEAT,
  Wrap_MIRRORED_REPEAT,
  Wrap_CLAMP_TO_EDGE,
  Wrap_CLAMP_TO_BORDER,
  Wrap_CLAMP_TO_ZERO,
};

enum FilterMode {
  Filter_NEAREST,
  Filter_LINEAR,
  Filter_NEAREST_MIPMAP_NEAREST,
  Filter_LINEAR_MIPMAP_NEAREST,
  Filter_NEAREST_MIPMAP_LINEAR,
  Filter_LINEAR_MIPMAP_LINEAR,
};

template<typename T>
class BaseTexture {
 public:

  static const char *TextureTypeString(TextureType type) {
#define TextureType_STR(type) case type: return #type
    switch (type) {
      TextureType_STR(TextureType_NONE);
      TextureType_STR(TextureType_DIFFUSE);
      TextureType_STR(TextureType_NORMALS);
      TextureType_STR(TextureType_EMISSIVE);
      TextureType_STR(TextureType_PBR_ALBEDO);
      TextureType_STR(TextureType_PBR_METAL_ROUGHNESS);
      TextureType_STR(TextureType_PBR_AMBIENT_OCCLUSION);

      TextureType_STR(TextureType_CUBE_MAP_POSITIVE_X);
      TextureType_STR(TextureType_CUBE_MAP_NEGATIVE_X);
      TextureType_STR(TextureType_CUBE_MAP_POSITIVE_Y);
      TextureType_STR(TextureType_CUBE_MAP_NEGATIVE_Y);
      TextureType_STR(TextureType_CUBE_MAP_POSITIVE_Z);
      TextureType_STR(TextureType_CUBE_MAP_NEGATIVE_Z);

      TextureType_STR(TextureType_CUBE);
      TextureType_STR(TextureType_EQUIRECTANGULAR);

      default:break;
    }
    return "";
  }

  static std::shared_ptr<Buffer<Vec4<T>>> CreateBufferDefault() {
#if SOFTGL_TEXTURE_TILED
    return std::make_shared<TiledBuffer<Vec4<T>>>();
#elif SOFTGL_TEXTURE_MORTON
    return std::make_shared<MortonBuffer<Vec4<T>>>();
#else
    return std::make_shared<Buffer<Vec4<T>>>();
#endif
  }

  static std::shared_ptr<Buffer<Vec4<T>>> CreateBuffer(BufferLayout layout) {
    switch (layout) {
      case Layout_Tiled: {
        return std::make_shared<TiledBuffer<Vec4<T>>>();
      }
      case Layout_Morton: {
        return std::make_shared<MortonBuffer<Vec4<T>>>();
      }
      case Layout_Linear:
      default: {
        return std::make_shared<Buffer<Vec4<T>>>();
      }
    }
  }

  BaseTexture &operator=(const BaseTexture &o) {
    if (this != &o) {
      type = o.type;
      buffer = o.buffer;
      mipmaps = o.mipmaps;
      mipmaps_ready = (bool)o.mipmaps_ready;
      mipmaps_generating = (bool)o.mipmaps_generating;
    }
    return *this;
  }

  virtual void Reset() {
    type = TextureType_NONE;
    buffer = nullptr;
    mipmaps.clear();
    mipmaps_ready = false;
    mipmaps_generating = false;
  }

  virtual ~BaseTexture() {
    if (mipmaps_thread) {
      mipmaps_thread->join();
    }
  };

 public:
  TextureType type = TextureType_NONE;
  std::shared_ptr<Buffer<Vec4<T>>> buffer = nullptr;

  std::vector<std::shared_ptr<Buffer<Vec4<T>>>> mipmaps;
  std::atomic<bool> mipmaps_ready{false};
  std::atomic<bool> mipmaps_generating{false};
  std::shared_ptr<std::thread> mipmaps_thread = nullptr;
};

using Texture = BaseTexture<uint8_t>;

}
