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
  FilterMode filterMin = Filter_NEAREST;
  FilterMode filterMag = Filter_NEAREST;

  WrapMode wrapS = Wrap_CLAMP_TO_EDGE;
  WrapMode wrapT = Wrap_CLAMP_TO_EDGE;
  WrapMode wrapR = Wrap_CLAMP_TO_EDGE;

  glm::vec4 borderColor = glm::vec4{0.f};
};

enum TextureType {
  TextureType_2D,
  TextureType_CUBE,
};

enum TextureFormat {
  TextureFormat_RGBA8 = 0,      // RGBA8888
  TextureFormat_FLOAT32 = 1,    // Float32
};

enum TextureUsage {
  TextureUsage_Sampler = 1 << 0,
  TextureUsage_AttachmentColor = 1 << 1,
  TextureUsage_AttachmentDepth = 1 << 2,
};

struct TextureDesc {
  int width = 0;
  int height = 0;
  TextureType type = TextureType_2D;
  TextureFormat format = TextureFormat_RGBA8;
  uint32_t usage = TextureUsage_Sampler;
  bool useMipmaps = false;
  bool multiSample = false;
};

class Texture : public TextureDesc {
 public:
  virtual int getId() const = 0;
  virtual void setSamplerDesc(SamplerDesc &sampler) {};
  virtual void initImageData() {};
  virtual void setImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) {};
  virtual void setImageData(const std::vector<std::shared_ptr<Buffer<float>>> &buffers) {};
  virtual void dumpImage(const char *path) {};
};

}
