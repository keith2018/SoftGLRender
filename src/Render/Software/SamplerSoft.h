/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <functional>
#include "TextureSoft.h"

namespace SoftGL {

#define CoordMod(i, n) ((i) & ((n) - 1) + (n)) & ((n) - 1)  // (i % n + n) % n
#define CoordMirror(i) (i) >= 0 ? (i) : (-1 - (i))

template<typename T>
class BaseSampler {
 public:
  virtual bool empty() = 0;
  inline size_t width() const { return width_; }
  inline size_t height() const { return height_; }
  inline T &borderColor() { return borderColor_; };

  T textureImpl(TextureImageSoft<T> *tex, glm::vec2 &uv, float lod = 0.f, glm::ivec2 offset = glm::ivec2(0));
  static T sampleNearest(Buffer<T> *buffer, glm::vec2 &uv, WrapMode wrap, glm::ivec2 &offset, T border);
  static T sampleBilinear(Buffer<T> *buffer, glm::vec2 &uv, WrapMode wrap, glm::ivec2 &offset, T border);

  static T pixelWithWrapMode(Buffer<T> *buffer, int x, int y, WrapMode wrap, T border);
  static void sampleBufferBilinear(Buffer<T> *buffer_out, Buffer<T> *buffer_in, T border);
  static T samplePixelBilinear(Buffer<T> *buffer, glm::vec2 uv, WrapMode wrap, T border);

  inline void setWrapMode(int wrap_mode) {
    wrapMode_ = (WrapMode) wrap_mode;
  }

  inline void setFilterMode(int filter_mode) {
    filterMode_ = (FilterMode) filter_mode;
  }

  inline void setLodFunc(std::function<float(BaseSampler<T> *)> *func) {
    lodFunc_ = func;
  }

  static void generateMipmaps(TextureImageSoft<T> *tex, bool sample);

 protected:
  T borderColor_;

  size_t width_ = 0;
  size_t height_ = 0;

  bool useMipmaps = false;
  WrapMode wrapMode_ = Wrap_CLAMP_TO_EDGE;
  FilterMode filterMode_ = Filter_LINEAR;
  std::function<float(BaseSampler<T> *)> *lodFunc_ = nullptr;
};

template<typename T>
class BaseSampler2D : public BaseSampler<T> {
 public:
  inline void setImage(TextureImageSoft<T> *tex) {
    tex_ = tex;
    BaseSampler<T>::width_ = (tex_ == nullptr) ? 0 : tex_->getWidth();
    BaseSampler<T>::height_ = (tex_ == nullptr) ? 0 : tex_->getHeight();
    BaseSampler<T>::useMipmaps = BaseSampler<T>::filterMode_ > Filter_LINEAR;
  }

  inline bool empty() override {
    return tex_ == nullptr;
  }

  T texture2DImpl(glm::vec2 &uv, float bias = 0.f) {
    float lod = bias;
    if (BaseSampler<T>::useMipmaps && BaseSampler<T>::lodFunc_) {
      lod += (*BaseSampler<T>::lodFunc_)(this);
    }
    return texture2DLodImpl(uv, lod);
  }

  T texture2DLodImpl(glm::vec2 &uv, float lod = 0.f, glm::ivec2 offset = glm::ivec2(0)) {
    return BaseSampler<T>::textureImpl(tex_, uv, lod, offset);
  }

 private:
  TextureImageSoft<T> *tex_ = nullptr;
};

template<typename T>
void BaseSampler<T>::generateMipmaps(TextureImageSoft<T> *tex, bool sample) {
  int width = tex->getWidth();
  int height = tex->getHeight();

  auto level0 = tex->getBuffer();
  tex->levels.resize(1);
  tex->levels[0] = level0;

  while (width > 2 && height > 2) {
    width = glm::max((int) glm::floor((float) width / 2.f), 1);
    height = glm::max((int) glm::floor((float) height / 2.f), 1);
    tex->levels.push_back(std::make_shared<ImageBufferSoft<T>>(width, height));
  }

  if (!sample) {
    return;
  }

  for (int i = 1; i < tex->levels.size(); i++) {
    BaseSampler<T>::sampleBufferBilinear(tex->levels[i]->buffer.get(), tex->levels[i - 1]->buffer.get(), T(0));
  }
}

template<typename T>
void TextureImageSoft<T>::generateMipmap(bool sample) {
  BaseSampler<T>::generateMipmaps(this, sample);
}

template<typename T>
T BaseSampler<T>::textureImpl(TextureImageSoft<T> *tex,
                              glm::vec2 &uv,
                              float lod,
                              glm::ivec2 offset) {
  if (tex != nullptr && !tex->empty()) {
    if (filterMode_ == Filter_NEAREST) {
      return sampleNearest(tex->levels[0]->buffer.get(), uv, wrapMode_, offset, borderColor_);
    }
    if (filterMode_ == Filter_LINEAR) {
      return sampleBilinear(tex->levels[0]->buffer.get(), uv, wrapMode_, offset, borderColor_);
    }

    // mipmaps
    int max_level = (int) tex->levels.size() - 1;

    if (filterMode_ == Filter_NEAREST_MIPMAP_NEAREST || filterMode_ == Filter_LINEAR_MIPMAP_NEAREST) {
      int level = glm::clamp((int) glm::ceil(lod + 0.5f) - 1, 0, max_level);
      if (filterMode_ == Filter_NEAREST_MIPMAP_NEAREST) {
        return sampleNearest(tex->levels[level]->buffer.get(), uv, wrapMode_, offset, borderColor_);
      } else {
        return sampleBilinear(tex->levels[level]->buffer.get(), uv, wrapMode_, offset, borderColor_);
      }
    }

    if (filterMode_ == Filter_NEAREST_MIPMAP_LINEAR || filterMode_ == Filter_LINEAR_MIPMAP_LINEAR) {
      int level_hi = glm::clamp((int) std::floor(lod), 0, max_level);
      int level_lo = glm::clamp(level_hi + 1, 0, max_level);

      T texel_hi, texel_lo;
      if (filterMode_ == Filter_NEAREST_MIPMAP_LINEAR) {
        texel_hi = sampleNearest(tex->levels[level_hi]->buffer.get(), uv, wrapMode_, offset, borderColor_);
      } else {
        texel_hi = sampleBilinear(tex->levels[level_hi]->buffer.get(), uv, wrapMode_, offset, borderColor_);
      }

      if (level_hi == level_lo) {
        return texel_hi;
      } else {
        if (filterMode_ == Filter_NEAREST_MIPMAP_LINEAR) {
          texel_lo = sampleNearest(tex->levels[level_lo]->buffer.get(), uv, wrapMode_, offset, borderColor_);
        } else {
          texel_lo = sampleBilinear(tex->levels[level_lo]->buffer.get(), uv, wrapMode_, offset, borderColor_);
        }
      }

      float f = glm::fract(lod);
      return glm::mix(texel_hi, texel_lo, f);
    }
  }
  return T(0);
}

template<typename T>
T BaseSampler<T>::pixelWithWrapMode(Buffer<T> *buffer, int x, int y, WrapMode wrap, T border) {
  int w = (int) buffer->getWidth();
  int h = (int) buffer->getHeight();
  switch (wrap) {
    case Wrap_REPEAT: {
      x = CoordMod(x, w);
      y = CoordMod(y, h);
      break;
    }
    case Wrap_MIRRORED_REPEAT: {
      x = CoordMod(x, 2 * w);
      y = CoordMod(y, 2 * h);

      x -= w;
      y -= h;

      x = CoordMirror(x);
      y = CoordMirror(y);

      x = w - 1 - x;
      y = h - 1 - y;
      break;
    }
    case Wrap_CLAMP_TO_EDGE: {
      if (x < 0) x = 0;
      if (y < 0) y = 0;
      if (x >= w) x = w - 1;
      if (y >= h) y = h - 1;
      break;
    }
    case Wrap_CLAMP_TO_BORDER: {
      if (x < 0 || x >= w) return border;
      if (y < 0 || y >= h) return border;
      break;
    }
    case Wrap_CLAMP_TO_ZERO: {
      if (x < 0 || x >= w) return T(0);
      if (y < 0 || y >= h) return T(0);
      break;
    }
  }

  T *ptr = buffer->get(x, y);
  if (ptr) {
    return *ptr;
  }
  return T(0);
}

template<typename T>
T BaseSampler<T>::sampleNearest(Buffer<T> *buffer,
                                glm::vec2 &uv,
                                WrapMode wrap,
                                glm::ivec2 &offset,
                                T border) {
  glm::vec2 texUV = uv * glm::vec2(buffer->getWidth(), buffer->getHeight());
  auto x = (int) glm::floor(texUV.x) + offset.x;
  auto y = (int) glm::floor(texUV.y) + offset.y;

  return pixelWithWrapMode(buffer, x, y, wrap, border);
}

template<typename T>
T BaseSampler<T>::sampleBilinear(Buffer<T> *buffer,
                                 glm::vec2 &uv,
                                 WrapMode wrap,
                                 glm::ivec2 &offset,
                                 T border) {
  glm::vec2 texUV = uv * glm::vec2(buffer->getWidth(), buffer->getHeight());
  texUV.x += (float) offset.x;
  texUV.y += (float) offset.y;
  return samplePixelBilinear(buffer, texUV, wrap, border);
}

template<typename T>
void BaseSampler<T>::sampleBufferBilinear(Buffer<T> *buffer_out, Buffer<T> *buffer_in, T border) {
  float ratio_x = (float) buffer_in->getWidth() / (float) buffer_out->getWidth();
  float ratio_y = (float) buffer_in->getHeight() / (float) buffer_out->getHeight();
  glm::vec2 delta = 0.5f * glm::vec2(ratio_x, ratio_y);
  for (int y = 0; y < (int) buffer_out->getHeight(); y++) {
    for (int x = 0; x < (int) buffer_out->getWidth(); x++) {
      glm::vec2 uv = glm::vec2((float) x * ratio_x, (float) y * ratio_y) + delta;
      auto color = samplePixelBilinear(buffer_in, uv, Wrap_CLAMP_TO_EDGE, border);
      buffer_out->set(x, y, color);
    }
  }
}

template<typename T>
T BaseSampler<T>::samplePixelBilinear(Buffer<T> *buffer, glm::vec2 uv, WrapMode wrap, T border) {
  auto x = (int) glm::floor(uv.x - 0.5f);
  auto y = (int) glm::floor(uv.y - 0.5f);

  auto s1 = pixelWithWrapMode(buffer, x, y, wrap, border);
  auto s2 = pixelWithWrapMode(buffer, x + 1, y, wrap, border);
  auto s3 = pixelWithWrapMode(buffer, x, y + 1, wrap, border);
  auto s4 = pixelWithWrapMode(buffer, x + 1, y + 1, wrap, border);

  glm::vec2 f = glm::fract(uv - glm::vec2(0.5f));
  return glm::mix(glm::mix(s1, s2, f.x), glm::mix(s3, s4, f.x), f.y);
}

template<typename T>
class BaseSamplerCube : public BaseSampler<T> {
 public:
  BaseSamplerCube() {
    BaseSampler<T>::wrapMode_ = Wrap_CLAMP_TO_EDGE;
    BaseSampler<T>::filterMode_ = Filter_LINEAR;
  }

  inline void setImage(TextureImageSoft<T> *tex, int idx) {
    texes_[idx] = tex;
    if (idx == 0) {
      BaseSampler<T>::width_ = (tex == nullptr) ? 0 : tex->getWidth();
      BaseSampler<T>::height_ = (tex == nullptr) ? 0 : tex->getHeight();
      BaseSampler<T>::useMipmaps = BaseSampler<T>::filterMode_ > Filter_LINEAR;
    }
  }

  inline bool empty() override {
    return texes_[0] == nullptr;
  }

  T textureCubeImpl(glm::vec3 &coord, float bias = 0.f) {
    float lod = bias;
    // cube sampler derivative not support
    // lod += dFd()...
    return textureCubeLodImpl(coord, lod);
  }

  T textureCubeLodImpl(glm::vec3 &coord, float lod = 0.f) {
    int index;
    glm::vec2 uv;
    convertXYZ2UV(coord.x, coord.y, coord.z, &index, &uv.x, &uv.y);
    TextureImageSoft<T> *tex = texes_[index];
    return BaseSampler<T>::textureImpl(tex, uv, lod);
  }

  static void convertXYZ2UV(float x, float y, float z, int *index, float *u, float *v);

 private:
  // +x, -x, +y, -y, +z, -z
  TextureImageSoft<T> *texes_[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
};

template<typename T>
void BaseSamplerCube<T>::convertXYZ2UV(float x, float y, float z, int *index, float *u, float *v) {
  // Ref: https://en.wikipedia.org/wiki/Cube_mapping
  float absX = std::fabs(x);
  float absY = std::fabs(y);
  float absZ = std::fabs(z);

  bool isXPositive = x > 0;
  bool isYPositive = y > 0;
  bool isZPositive = z > 0;

  float maxAxis, uc, vc;

  // POSITIVE X
  if (isXPositive && absX >= absY && absX >= absZ) {
    maxAxis = absX;
    uc = -z;
    vc = y;
    *index = 0;
  }
  // NEGATIVE X
  if (!isXPositive && absX >= absY && absX >= absZ) {
    maxAxis = absX;
    uc = z;
    vc = y;
    *index = 1;
  }
  // POSITIVE Y
  if (isYPositive && absY >= absX && absY >= absZ) {
    maxAxis = absY;
    uc = x;
    vc = -z;
    *index = 2;
  }
  // NEGATIVE Y
  if (!isYPositive && absY >= absX && absY >= absZ) {
    maxAxis = absY;
    uc = x;
    vc = z;
    *index = 3;
  }
  // POSITIVE Z
  if (isZPositive && absZ >= absX && absZ >= absY) {
    maxAxis = absZ;
    uc = x;
    vc = y;
    *index = 4;
  }
  // NEGATIVE Z
  if (!isZPositive && absZ >= absX && absZ >= absY) {
    maxAxis = absZ;
    uc = -x;
    vc = y;
    *index = 5;
  }

  // flip y
  vc = -vc;

  // Convert range from -1 to 1 to 0 to 1
  *u = 0.5f * (uc / maxAxis + 1.0f);
  *v = 0.5f * (vc / maxAxis + 1.0f);
}

class SamplerSoft {
 public:
  virtual TextureType texType() = 0;
  virtual void setTexture(const std::shared_ptr<Texture> &tex) = 0;
};

template<typename T>
class Sampler2DSoft : public SamplerSoft {
 public:
  TextureType texType() override {
    return TextureType_2D;
  }

  void setTexture(const std::shared_ptr<Texture> &tex) override {
    tex_ = dynamic_cast<Texture2DSoft<T> *>(tex.get());
    tex_->getBorderColor(sampler_.borderColor());
    sampler_.setFilterMode(tex_->getSamplerDesc().filterMin);
    sampler_.setWrapMode(tex_->getSamplerDesc().wrapS);
    sampler_.setImage(&tex_->getImage());
  }

  inline Texture2DSoft<T> *getTexture() const {
    return tex_;
  }

  inline void setLodFunc(std::function<float(BaseSampler<T> *)> *func) {
    sampler_.setLodFunc(func);
  }

  inline T texture2D(glm::vec2 coord, float bias = 0.f) {
    return sampler_.texture2DImpl(coord, bias);
  }

  inline T texture2DLod(glm::vec2 coord, float lod = 0.f) {
    return sampler_.texture2DLodImpl(coord, lod);
  }

  inline T texture2DLodOffset(glm::vec2 coord, float lod, glm::ivec2 offset) {
    return sampler_.texture2DLodImpl(coord, lod, offset);
  }

 private:
  BaseSampler2D<T> sampler_;
  Texture2DSoft<T> *tex_ = nullptr;
};

template<typename T>
class SamplerCubeSoft : public SamplerSoft {
 public:
  TextureType texType() override {
    return TextureType_CUBE;
  }

  void setTexture(const std::shared_ptr<Texture> &tex) override {
    tex_ = dynamic_cast<TextureCubeSoft<T> *>(tex.get());
    tex_->getBorderColor(sampler_.borderColor());
    sampler_.setFilterMode(tex_->getSamplerDesc().filterMin);
    sampler_.setWrapMode(tex_->getSamplerDesc().wrapS);
    for (int i = 0; i < 6; i++) {
      sampler_.setImage(&tex_->getImage((CubeMapFace) i), i);
    }
  }

  inline TextureCubeSoft<T> *getTexture() const {
    return tex_;
  }

  inline T textureCube(glm::vec3 coord, float bias = 0.f) {
    return sampler_.textureCubeImpl(coord, bias);
  }

  inline T textureCubeLod(glm::vec3 coord, float lod = 0.f) {
    return sampler_.textureCubeLodImpl(coord, lod);
  }

 private:
  BaseSamplerCube<T> sampler_;
  TextureCubeSoft<T> *tex_ = nullptr;
};

}
