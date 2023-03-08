/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <memory>
#include <vector>
#include "base/buffer.h"
#include "base/glm_inc.h"

namespace SoftGL {

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

enum CubeMapFace {
  TEXTURE_CUBE_MAP_POSITIVE_X = 0,
  TEXTURE_CUBE_MAP_NEGATIVE_X = 1,
  TEXTURE_CUBE_MAP_POSITIVE_Y = 2,
  TEXTURE_CUBE_MAP_NEGATIVE_Y = 3,
  TEXTURE_CUBE_MAP_POSITIVE_Z = 4,
  TEXTURE_CUBE_MAP_NEGATIVE_Z = 5,
};

struct SamplerDesc {
  bool use_mipmaps = false;
  virtual ~SamplerDesc() = default;
};

struct Sampler2DDesc : SamplerDesc {
  WrapMode wrap_s;
  WrapMode wrap_t;
  FilterMode filter_min;
  FilterMode filter_mag;

  Sampler2DDesc() {
    use_mipmaps = false;
    wrap_s = Wrap_CLAMP_TO_EDGE;
    wrap_t = Wrap_CLAMP_TO_EDGE;
    filter_min = Filter_LINEAR;
    filter_mag = Filter_LINEAR;
  }
};

struct SamplerCubeDesc : Sampler2DDesc {
  WrapMode wrap_r;

  SamplerCubeDesc() {
    use_mipmaps = false;
    wrap_s = Wrap_CLAMP_TO_EDGE;
    wrap_t = Wrap_CLAMP_TO_EDGE;
    wrap_r = Wrap_CLAMP_TO_EDGE;
    filter_min = Filter_LINEAR;
    filter_mag = Filter_LINEAR;
  }
};

enum TextureType {
  TextureType_2D,
  TextureType_CUBE,
};

enum TextureFormat {
  TextureFormat_RGBA8 = 0,    // RGBA8888
  TextureFormat_DEPTH = 1,    // Float32
};

struct TextureDesc {
  TextureDesc(TextureType type, TextureFormat format, bool multi_sample)
      : type(type), format(format), multi_sample(multi_sample) {}

  TextureType type = TextureType_2D;
  TextureFormat format = TextureFormat_RGBA8;
  bool multi_sample = false;
};

class Texture {
 public:
  virtual int GetId() const = 0;
  virtual void SetSamplerDesc(SamplerDesc &sampler) {};
  virtual void SetImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) {};
  virtual void SetImageData(const std::vector<std::shared_ptr<Buffer<float>>> &buffers) {};
  virtual void InitImageData(int w, int h) {};
  virtual void DumpImage(const char *path) {};

 public:
  int width = 0;
  int height = 0;
  bool multi_sample = false;
  TextureType type = TextureType_2D;
  TextureFormat format = TextureFormat_RGBA8;
};

}
