/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <memory>
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
  TEXTURE_CUBE_MAP_POSITIVE_X,
  TEXTURE_CUBE_MAP_NEGATIVE_X,
  TEXTURE_CUBE_MAP_POSITIVE_Y,
  TEXTURE_CUBE_MAP_NEGATIVE_Y,
  TEXTURE_CUBE_MAP_POSITIVE_Z,
  TEXTURE_CUBE_MAP_NEGATIVE_Z,
};

struct Sampler2D {
  bool use_mipmaps = false;
  WrapMode wrap_s = Wrap_REPEAT;
  WrapMode wrap_t = Wrap_REPEAT;
  FilterMode filter_min = Filter_LINEAR;
  FilterMode filter_mag = Filter_LINEAR;
};

struct SamplerCube : Sampler2D {
  WrapMode wrap_r = Wrap_REPEAT;
};

class Texture {
 public:
  int width = 0;
  int height = 0;
  bool use_mipmaps = false;
};

class Texture2D : public Texture {
 public:
  virtual void SetSampler(Sampler2D &sampler) = 0;
  virtual void SetImageData(BufferRGBA &buffer) = 0;
  virtual void InitImageData(int width, int height) = 0;
};

class TextureDepth: public Texture {
 public:
  virtual void InitImageData(int width, int height) = 0;
};

class TextureCube : public Texture {
 public:
  virtual void SetSampler(SamplerCube &sampler) = 0;
  virtual void SetImageData(BufferRGBA &buffer, CubeMapFace face) = 0;
  virtual void InitImageData(int width, int height, CubeMapFace face) = 0;

 public:
  std::shared_ptr<Texture2D> faces[6];  // +x, -x, +y, -y, +z, -z
};

}