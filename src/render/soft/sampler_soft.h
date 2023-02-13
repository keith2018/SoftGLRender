/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/simd.h"
#include "texture_soft.h"

namespace SoftGL {

#define CoordMod(i, n) ((i) & ((n) - 1) + (n)) & ((n) - 1)  // (i % n + n) % n
#define CoordMirror(i) (i) >= 0 ? (i) : (-1 - (i))

template<typename T>
class BaseSampler {
 public:
  virtual bool Empty() = 0;
  inline size_t Width() const { return width_; }
  inline size_t Height() const { return height_; }
  inline void SetLodFunc(std::function<float(BaseSampler<T> &)> *func) { lod_func_ = func; }

  glm::tvec4<T> TextureImpl(TextureImageSoft<T> *tex,
                            glm::vec2 &uv,
                            float lod = 0.f,
                            glm::ivec2 offset = glm::ivec2(0));
  static glm::tvec4<T> SampleNearest(Buffer<glm::tvec4<T>> *buffer, glm::vec2 &uv, WrapMode wrap, glm::ivec2 &offset);
  static glm::tvec4<T> SampleBilinear(Buffer<glm::tvec4<T>> *buffer, glm::vec2 &uv, WrapMode wrap, glm::ivec2 &offset);

  static glm::tvec4<T> PixelWithWrapMode(Buffer<glm::tvec4<T>> *buffer, int x, int y, WrapMode wrap);
  static void SampleBufferBilinear(Buffer<glm::tvec4<T>> *buffer_out, Buffer<glm::tvec4<T>> *buffer_in);
  static glm::tvec4<T> SamplePixelBilinear(Buffer<glm::tvec4<T>> *buffer, glm::vec2 uv, WrapMode wrap);

  inline void SetWrapMode(int wrap_mode) {
    wrap_mode_ = (WrapMode) wrap_mode;
  }

  inline void SetFilterMode(int filter_mode) {
    filter_mode_ = (FilterMode) filter_mode;
  }

  static int RoundupPowerOf2(int n) {
    static const int max = 1 << 30;
    n -= 1;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n < 0 ? 1 : (n >= max ? max : n + 1);
  }

 public:
  static const glm::tvec4<T> BORDER_COLOR;

 protected:
  size_t width_ = 0;
  size_t height_ = 0;

  WrapMode wrap_mode_ = Wrap_REPEAT;
  FilterMode filter_mode_ = Filter_LINEAR;
  std::function<float(BaseSampler<T> &)> *lod_func_ = nullptr;
};

template<typename T>
class BaseSampler2D : public BaseSampler<T> {
 public:
  inline void SetTexture(TextureImageSoft<T> *tex) {
    tex_ = tex;
    BaseSampler<T>::width_ = (tex_ == nullptr || tex_->buffer->Empty()) ? 0 : tex_->buffer->GetWidth();
    BaseSampler<T>::height_ = (tex_ == nullptr || tex_->buffer->Empty()) ? 0 : tex_->buffer->GetHeight();
  }

  inline bool Empty() override {
    return tex_ == nullptr;
  }

  virtual glm::vec4 Texture2DImpl(glm::vec2 &uv, float bias = 0.f) {
    float lod = bias;
    if (BaseSampler<T>::lod_func_) {
      lod += (*BaseSampler<T>::lod_func_)(*this);
    }
    return Texture2DLodImpl(uv, lod);
  }

  virtual glm::vec4 Texture2DLodImpl(glm::vec2 &uv, float lod = 0.f, glm::ivec2 offset = glm::ivec2(0)) {
    if (tex_ == nullptr) {
      return {0, 0, 0, 0};
    }
    glm::tvec4<T> color = BaseSampler<T>::TextureImpl(tex_, uv, lod, offset);
    return {color[0], color[1], color[2], color[3]};
  }

 private:
  TextureImageSoft<T> *tex_ = nullptr;
};

template<typename T>
const glm::tvec4<T> BaseSampler<T>::BORDER_COLOR(0);

template<typename T>
glm::tvec4<T> BaseSampler<T>::TextureImpl(TextureImageSoft<T> *tex, glm::vec2 &uv, float lod, glm::ivec2 offset) {
  if (tex != nullptr && !tex->buffer->Empty()) {
    if (filter_mode_ == Filter_NEAREST) {
      return SampleNearest(tex->buffer.get(), uv, wrap_mode_, offset);
    }
    if (filter_mode_ == Filter_LINEAR) {
      return SampleBilinear(tex->buffer.get(), uv, wrap_mode_, offset);
    }

    if (filter_mode_ == Filter_NEAREST_MIPMAP_NEAREST || filter_mode_ == Filter_NEAREST_MIPMAP_LINEAR) {
      return SampleNearest(tex->buffer.get(), uv, wrap_mode_, offset);
    }
    if (filter_mode_ == Filter_LINEAR_MIPMAP_NEAREST || filter_mode_ == Filter_LINEAR_MIPMAP_LINEAR) {
      return SampleBilinear(tex->buffer.get(), uv, wrap_mode_, offset);
    }
  }
  return glm::tvec4<T>(0);
}

template<typename T>
glm::tvec4<T> BaseSampler<T>::PixelWithWrapMode(Buffer<glm::tvec4<T>> *buffer, int x, int y, WrapMode wrap) {
  int w = (int) buffer->GetWidth();
  int h = (int) buffer->GetHeight();
  switch (wrap) {
    case Wrap_REPEAT: {
      x = CoordMod(x, w);
      y = CoordMod(y, h);
    }
      break;
    case Wrap_MIRRORED_REPEAT: {
      x = CoordMod(x, 2 * w);
      y = CoordMod(y, 2 * h);

      x -= w;
      y -= h;

      x = CoordMirror(x);
      y = CoordMirror(y);

      x = w - 1 - x;
      y = h - 1 - y;
    }
      break;
    case Wrap_CLAMP_TO_EDGE: {
      if (x < 0) x = 0;
      if (y < 0) y = 0;
      if (x >= w) x = w - 1;
      if (y >= h) y = h - 1;
    }
      break;
    case Wrap_CLAMP_TO_BORDER: {
      if (x < 0 || x >= w) return BORDER_COLOR;
      if (y < 0 || y >= h) return BORDER_COLOR;
    }
      break;
    case Wrap_CLAMP_TO_ZERO: {
      if (x < 0 || x >= w) return glm::tvec4<T>(0);
      if (y < 0 || y >= h) return glm::tvec4<T>(0);
    }
      break;
  }

  glm::tvec4<T> *ptr = buffer->Get(x, y);
  if (ptr) {
    return *ptr;
  }
  return glm::tvec4<T>(0);
}

template<typename T>
glm::tvec4<T> BaseSampler<T>::SampleNearest(Buffer<glm::tvec4<T>> *buffer,
                                            glm::vec2 &uv,
                                            WrapMode wrap,
                                            glm::ivec2 &offset) {
  glm::vec2 texUV = uv * glm::vec2(buffer->GetWidth(), buffer->GetHeight());
  auto x = (int) texUV.x + offset.x;
  auto y = (int) texUV.y + offset.y;

  return PixelWithWrapMode(buffer, x, y, wrap);
}

template<typename T>
glm::tvec4<T> BaseSampler<T>::SampleBilinear(Buffer<glm::tvec4<T>> *buffer,
                                             glm::vec2 &uv,
                                             WrapMode wrap,
                                             glm::ivec2 &offset) {
  glm::vec2 texUV = uv * glm::vec2(buffer->GetWidth(), buffer->GetHeight()) - glm::vec2(0.5f);
  texUV.x += offset.x;
  texUV.y += offset.y;
  return SamplePixelBilinear(buffer, texUV, wrap);
}

template<typename T>
void BaseSampler<T>::SampleBufferBilinear(Buffer<glm::tvec4<T>> *buffer_out, Buffer<glm::tvec4<T>> *buffer_in) {
  float ratio_x = (float) buffer_in->GetWidth() / (float) buffer_out->GetWidth();
  float ratio_y = (float) buffer_in->GetHeight() / (float) buffer_out->GetHeight();
  for (int y = 0; y < (int) buffer_out->GetHeight(); y++) {
    for (int x = 0; x < (int) buffer_out->GetWidth(); x++) {
      glm::vec2 uv = glm::vec2((float) x * ratio_x, (float) y * ratio_y);
      auto color = SamplePixelBilinear(buffer_in, uv, Wrap_CLAMP_TO_EDGE);
      buffer_out->Set(x, y, color);
    }
  }
}

template<typename T>
glm::tvec4<T> BaseSampler<T>::SamplePixelBilinear(Buffer<glm::tvec4<T>> *buffer, glm::vec2 uv, WrapMode wrap) {
  auto x = (int) uv.x;
  auto y = (int) uv.y;

  auto s1 = PixelWithWrapMode(buffer, x, y, wrap);
  auto s2 = PixelWithWrapMode(buffer, x + 1, y, wrap);
  auto s3 = PixelWithWrapMode(buffer, x, y + 1, wrap);
  auto s4 = PixelWithWrapMode(buffer, x + 1, y + 1, wrap);

  glm::vec2 f = glm::fract(uv);

#ifdef SOFTGL_SIMD_OPT
  __m128 s1_ = _mm_set_ps(s1[3], s1[2], s1[1], s1[0]);
  __m128 s2_ = _mm_set_ps(s2[3], s2[2], s2[1], s2[0]);
  __m128 s3_ = _mm_set_ps(s3[3], s3[2], s3[1], s3[0]);
  __m128 s4_ = _mm_set_ps(s4[3], s4[2], s4[1], s4[0]);

  __m128 fx_ = _mm_set1_ps(f.x);
  __m128 fy_ = _mm_set1_ps(f.y);

  __m128 r1 = _mm_fmadd_ps(fx_, s2_, _mm_fnmadd_ps(fx_, s1_, s1_));
  __m128 r2 = _mm_fmadd_ps(fx_, s4_, _mm_fnmadd_ps(fx_, s3_, s3_));
  __m128 ret = _mm_fmadd_ps(fy_, r2, _mm_fnmadd_ps(fy_, r1, r1));

  return {MM_F32(ret, 0), MM_F32(ret, 1), MM_F32(ret, 2), MM_F32(ret, 3)};
#else
  return glm::mix(glm::mix(s1, s2, f.x), glm::mix(s3, s4, f.x), f.y);
#endif
}

class Sampler2DImpl : public BaseSampler2D<uint8_t> {
 public:

  glm::vec4 Texture2D(glm::vec2 uv, float bias = 0.f) {
    return BaseSampler2D::Texture2DImpl(uv, bias) / 255.f;
  }

  glm::vec4 Texture2DLod(glm::vec2 uv, float lod = 0.f) {
    return BaseSampler2D::Texture2DLodImpl(uv, lod) / 255.f;
  }

  glm::vec4 Texture2DLodOffset(glm::vec2 uv, float lod, glm::ivec2 offset) {
    return BaseSampler2D::Texture2DLodImpl(uv, lod, offset) / 255.f;
  }
};

class SamplerSoft {
 public:
  virtual TextureType Type() = 0;
  virtual void SetTexture(const std::shared_ptr<Texture> &tex) = 0;
};

class Sampler2DSoft : public SamplerSoft {
 public:
  TextureType Type() override {
    return TextureType_2D;
  }

  void SetTexture(const std::shared_ptr<Texture> &tex) override {
    tex_ = dynamic_cast<Texture2DSoft *>(tex.get());
    sampler_.SetTexture(&tex_->GetImage());
    sampler_.SetFilterMode(tex_->GetSamplerDesc().filter_min);
    sampler_.SetWrapMode(tex_->GetSamplerDesc().wrap_s);
  }

  inline glm::vec4 Texture2D(glm::vec2 coord, float bias = 0.f) {
    return sampler_.Texture2D(coord, bias);
  }

  inline glm::vec4 Texture2DLod(glm::vec2 coord, float lod = 0.f) {
    return sampler_.Texture2DLod(coord, lod);
  }

  inline glm::vec4 Texture2DLodOffset(glm::vec2 coord, float lod, glm::ivec2 offset) {
    return sampler_.Texture2DLodOffset(coord, lod, offset);
  }

 private:
  Sampler2DImpl sampler_;
  Texture2DSoft *tex_ = nullptr;
};

class SamplerCubeSoft : public SamplerSoft {
 public:
  TextureType Type() override {
    return TextureType_CUBE;
  }

  void SetTexture(const std::shared_ptr<Texture> &tex) override {
    // TODO
  }

  inline glm::vec4 TextureCube(glm::vec3 coord, float bias = 0.f) {
    return {};
  }

  inline glm::vec4 TextureCubeLod(glm::vec3 coord, float lod = 0.f) {
    return {};
  }
};

}
