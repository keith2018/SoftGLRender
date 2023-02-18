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

  inline void SetLodFunc(std::function<float(BaseSampler<T> *)> *func) {
    lod_func_ = func;
  }

  static void GenerateMipmaps(TextureImageSoft<T> *tex, bool sample);

 public:
  static const glm::tvec4<T> BORDER_COLOR;

 protected:
  size_t width_ = 0;
  size_t height_ = 0;

  bool use_mipmaps = false;
  WrapMode wrap_mode_ = Wrap_REPEAT;
  FilterMode filter_mode_ = Filter_LINEAR;
  std::function<float(BaseSampler<T> *)> *lod_func_ = nullptr;
};

template<typename T>
class BaseSampler2D : public BaseSampler<T> {
 public:
  inline void SetImage(TextureImageSoft<T> *tex) {
    tex_ = tex;
    BaseSampler<T>::width_ = (tex_ == nullptr) ? 0 : tex_->GetWidth();
    BaseSampler<T>::height_ = (tex_ == nullptr) ? 0 : tex_->GetHeight();
    BaseSampler<T>::use_mipmaps = BaseSampler<T>::filter_mode_ > Filter_LINEAR;
  }

  inline bool Empty() override {
    return tex_ == nullptr;
  }

  glm::vec4 Texture2DImpl(glm::vec2 &uv, float bias = 0.f) {
    float lod = bias;
    if (BaseSampler<T>::use_mipmaps && BaseSampler<T>::lod_func_) {
      lod += (*BaseSampler<T>::lod_func_)(this);
    }
    return Texture2DLodImpl(uv, lod);
  }

  glm::vec4 Texture2DLodImpl(glm::vec2 &uv, float lod = 0.f, glm::ivec2 offset = glm::ivec2(0)) {
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
void BaseSampler<T>::GenerateMipmaps(TextureImageSoft<T> *tex, bool sample) {
  int width = tex->GetWidth();
  int height = tex->GetHeight();

  auto level0 = tex->GetBuffer();
  tex->levels.resize(1);
  tex->levels[0] = level0;

  while (width > 2 && height > 2) {
    width = glm::max((int) glm::floor((float) width / 2.f), 1);
    height = glm::max((int) glm::floor((float) height / 2.f), 1);
    tex->levels.push_back(Buffer<glm::tvec4<T>>::MakeDefault(width, height));
  }

  if (!sample) {
    return;
  }

  for (int i = 1; i < tex->levels.size(); i++) {
    BaseSampler<T>::SampleBufferBilinear(tex->levels[i].get(), tex->levels[i - 1].get());
  }
}

template<typename T>
void TextureImageSoft<T>::GenerateMipmap(bool sample) {
  BaseSampler<T>::GenerateMipmaps(this, sample);
}

template<typename T>
glm::tvec4<T> BaseSampler<T>::TextureImpl(TextureImageSoft<T> *tex,
                                          glm::vec2 &uv,
                                          float lod,
                                          glm::ivec2 offset) {
  if (tex != nullptr && !tex->Empty()) {
    if (filter_mode_ == Filter_NEAREST) {
      return SampleNearest(tex->levels[0].get(), uv, wrap_mode_, offset);
    }
    if (filter_mode_ == Filter_LINEAR) {
      return SampleBilinear(tex->levels[0].get(), uv, wrap_mode_, offset);
    }

    // mipmaps
    int max_level = (int) tex->levels.size() - 1;

    if (filter_mode_ == Filter_NEAREST_MIPMAP_NEAREST || filter_mode_ == Filter_LINEAR_MIPMAP_NEAREST) {
      int level = glm::clamp((int) glm::ceil(lod + 0.5f) - 1, 0, max_level);
      if (filter_mode_ == Filter_NEAREST_MIPMAP_NEAREST) {
        return SampleNearest(tex->levels[level].get(), uv, wrap_mode_, offset);
      } else {
        return SampleBilinear(tex->levels[level].get(), uv, wrap_mode_, offset);
      }
    }

    if (filter_mode_ == Filter_NEAREST_MIPMAP_LINEAR || filter_mode_ == Filter_LINEAR_MIPMAP_LINEAR) {
      int level_hi = glm::clamp((int) std::floor(lod), 0, max_level);
      int level_lo = glm::clamp(level_hi + 1, 0, max_level);

      glm::tvec4<T> texel_hi, texel_lo;
      if (filter_mode_ == Filter_NEAREST_MIPMAP_LINEAR) {
        texel_hi = SampleNearest(tex->levels[level_hi].get(), uv, wrap_mode_, offset);
      } else {
        texel_hi = SampleBilinear(tex->levels[level_hi].get(), uv, wrap_mode_, offset);
      }

      if (level_hi == level_lo) {
        return texel_hi;
      } else {
        if (filter_mode_ == Filter_NEAREST_MIPMAP_LINEAR) {
          texel_lo = SampleNearest(tex->levels[level_lo].get(), uv, wrap_mode_, offset);
        } else {
          texel_lo = SampleBilinear(tex->levels[level_lo].get(), uv, wrap_mode_, offset);
        }
      }

      float f = glm::fract(lod);
      return glm::mix(texel_hi, texel_lo, f);
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
  auto x = (int) glm::floor(texUV.x) + offset.x;
  auto y = (int) glm::floor(texUV.y) + offset.y;

  return PixelWithWrapMode(buffer, x, y, wrap);
}

template<typename T>
glm::tvec4<T> BaseSampler<T>::SampleBilinear(Buffer<glm::tvec4<T>> *buffer,
                                             glm::vec2 &uv,
                                             WrapMode wrap,
                                             glm::ivec2 &offset) {
  glm::vec2 texUV = uv * glm::vec2(buffer->GetWidth(), buffer->GetHeight());
  texUV.x += (float) offset.x;
  texUV.y += (float) offset.y;
  return SamplePixelBilinear(buffer, texUV, wrap);
}

template<typename T>
void BaseSampler<T>::SampleBufferBilinear(Buffer<glm::tvec4<T>> *buffer_out, Buffer<glm::tvec4<T>> *buffer_in) {
  float ratio_x = (float) buffer_in->GetWidth() / (float) buffer_out->GetWidth();
  float ratio_y = (float) buffer_in->GetHeight() / (float) buffer_out->GetHeight();
  glm::vec2 delta = 0.5f * glm::vec2(ratio_x, ratio_y);
  for (int y = 0; y < (int) buffer_out->GetHeight(); y++) {
    for (int x = 0; x < (int) buffer_out->GetWidth(); x++) {
      glm::vec2 uv = glm::vec2((float) x * ratio_x, (float) y * ratio_y) + delta;
      auto color = SamplePixelBilinear(buffer_in, uv, Wrap_CLAMP_TO_EDGE);
      buffer_out->Set(x, y, color);
    }
  }
}

template<typename T>
glm::tvec4<T> BaseSampler<T>::SamplePixelBilinear(Buffer<glm::tvec4<T>> *buffer, glm::vec2 uv, WrapMode wrap) {
  auto x = (int) glm::floor(uv.x - 0.5f);
  auto y = (int) glm::floor(uv.y - 0.5f);

  auto s1 = PixelWithWrapMode(buffer, x, y, wrap);
  auto s2 = PixelWithWrapMode(buffer, x + 1, y, wrap);
  auto s3 = PixelWithWrapMode(buffer, x, y + 1, wrap);
  auto s4 = PixelWithWrapMode(buffer, x + 1, y + 1, wrap);

  glm::vec2 f = glm::fract(uv - glm::vec2(0.5f));

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

template<typename T>
class BaseSamplerCube : public BaseSampler<T> {
 public:
  BaseSamplerCube() {
    BaseSampler<T>::wrap_mode_ = Wrap_CLAMP_TO_EDGE;
    BaseSampler<T>::filter_mode_ = Filter_LINEAR;
  }

  inline void SetImage(TextureImageSoft<T> *tex, int idx) {
    texes_[idx] = tex;
    if (idx == 0) {
      BaseSampler<T>::width_ = (tex == nullptr) ? 0 : tex->GetWidth();
      BaseSampler<T>::height_ = (tex == nullptr) ? 0 : tex->GetHeight();
      BaseSampler<T>::use_mipmaps = BaseSampler<T>::filter_mode_ > Filter_LINEAR;
    }
  }

  inline bool Empty() override {
    return texes_[0] == nullptr;
  }

  glm::vec4 TextureCubeImpl(glm::vec3 &coord, float bias = 0.f) {
    float lod = bias;
    // cube sampler derivative not support
    // lod += dFd()...
    return TextureCubeLodImpl(coord, lod);
  }

  glm::vec4 TextureCubeLodImpl(glm::vec3 &coord, float lod = 0.f) {
    int index;
    glm::vec2 uv;
    ConvertXYZ2UV(coord.x, coord.y, coord.z, &index, &uv.x, &uv.y);
    TextureImageSoft<T> *tex = texes_[index];
    if (tex == nullptr) {
      return {0, 0, 0, 0};
    }
    glm::tvec4<T> color = BaseSampler<T>::TextureImpl(tex, uv, lod);
    return {color[0], color[1], color[2], color[3]};
  }

  static void ConvertXYZ2UV(float x, float y, float z, int *index, float *u, float *v);

 private:
  // +x, -x, +y, -y, +z, -z
  TextureImageSoft<T> *texes_[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
};

template<typename T>
void BaseSamplerCube<T>::ConvertXYZ2UV(float x, float y, float z, int *index, float *u, float *v) {
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

class Sampler2DImpl : public BaseSampler2D<uint8_t> {
 public:
  inline glm::vec4 Texture2D(glm::vec2 uv, float bias = 0.f) {
    return BaseSampler2D::Texture2DImpl(uv, bias) / 255.f;
  }

  inline glm::vec4 Texture2DLod(glm::vec2 uv, float lod = 0.f) {
    return BaseSampler2D::Texture2DLodImpl(uv, lod) / 255.f;
  }

  inline glm::vec4 Texture2DLodOffset(glm::vec2 uv, float lod, glm::ivec2 offset) {
    return BaseSampler2D::Texture2DLodImpl(uv, lod, offset) / 255.f;
  }
};

class SamplerCubeImpl : public BaseSamplerCube<uint8_t> {
 public:
  inline glm::vec4 TextureCube(glm::vec3 coord, float bias = 0.f) {
    return BaseSamplerCube::TextureCubeImpl(coord, bias) / 255.f;
  }

  inline glm::vec4 TextureCubeLod(glm::vec3 coord, float lod = 0.f) {
    return BaseSamplerCube::TextureCubeLodImpl(coord, lod) / 255.f;
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
    sampler_.SetFilterMode(tex_->GetSamplerDesc().filter_min);
    sampler_.SetWrapMode(tex_->GetSamplerDesc().wrap_s);
    sampler_.SetImage(&tex_->GetImage());
  }

  inline Texture2DSoft *GetTexture() const {
    return tex_;
  }

  inline void SetLodFunc(std::function<float(BaseSampler<uint8_t> *)> *func) {
    sampler_.SetLodFunc(func);
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
    tex_ = dynamic_cast<TextureCubeSoft *>(tex.get());
    sampler_.SetFilterMode(tex_->GetSamplerDesc().filter_min);
    sampler_.SetWrapMode(tex_->GetSamplerDesc().wrap_s);
    for (int i = 0; i < 6; i++) {
      sampler_.SetImage(&tex_->GetImage((CubeMapFace) i), i);
    }
  }

  inline TextureCubeSoft *GetTexture() const {
    return tex_;
  }

  inline glm::vec4 TextureCube(glm::vec3 coord, float bias = 0.f) {
    return sampler_.TextureCube(coord, bias);
  }

  inline glm::vec4 TextureCubeLod(glm::vec3 coord, float lod = 0.f) {
    return sampler_.TextureCubeLod(coord, lod);
  }

 private:
  SamplerCubeImpl sampler_;
  TextureCubeSoft *tex_ = nullptr;
};

}
