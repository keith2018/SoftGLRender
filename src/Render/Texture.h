/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <memory>
#include <vector>
#include "Base/Buffer.h"
#include "Base/GLMInc.h"

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
  bool useMipmaps = false;
  virtual ~SamplerDesc() = default;
};

struct Sampler2DDesc : SamplerDesc {
  WrapMode wrapS;
  WrapMode wrapT;
  FilterMode filterMin;
  FilterMode filterMag;
  glm::vec4 borderColor{0.f};

  Sampler2DDesc() {
    useMipmaps = false;
    wrapS = Wrap_CLAMP_TO_EDGE;
    wrapT = Wrap_CLAMP_TO_EDGE;
    filterMin = Filter_LINEAR;
    filterMag = Filter_LINEAR;
  }
};

struct SamplerCubeDesc : Sampler2DDesc {
  WrapMode wrapR;

  SamplerCubeDesc() {
    useMipmaps = false;
    wrapS = Wrap_CLAMP_TO_EDGE;
    wrapT = Wrap_CLAMP_TO_EDGE;
    wrapR = Wrap_CLAMP_TO_EDGE;
    filterMin = Filter_LINEAR;
    filterMag = Filter_LINEAR;
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
  TextureDesc(TextureType type, TextureFormat format, bool multiSample)
      : type(type), format(format), multiSample(multiSample) {}

  TextureType type = TextureType_2D;
  TextureFormat format = TextureFormat_RGBA8;
  bool multiSample = false;
};

class Texture {
 public:
  virtual int getId() const = 0;
  virtual void setSamplerDesc(SamplerDesc &sampler) {};
  virtual void setImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) {};
  virtual void setImageData(const std::vector<std::shared_ptr<Buffer<float>>> &buffers) {};
  virtual void initImageData(int w, int h) {};
  virtual void dumpImage(const char *path) {};

 public:
  int width = 0;
  int height = 0;
  bool multiSample = false;
  TextureType type = TextureType_2D;
  TextureFormat format = TextureFormat_RGBA8;
};

}
