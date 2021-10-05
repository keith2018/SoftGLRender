/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <utility>
#include <functional>
#include <atomic>

#include "FrameBuffer.h"

namespace SoftGL {

#define Vec4 glm::tvec4

#define CoordMod(i, n) ((i) & ((n) - 1) + (n)) & ((n) - 1)  // (i % n + n) % n
#define CoordMirror(i) (i) >= 0 ? (i) : (-1 - (i))

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

  TextureType_EQUIRECTANGULAR,
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

      TextureType_STR(TextureType_EQUIRECTANGULAR);

      default:break;
    }
    return "";
  }

  void GenerateMipmaps();

  bool MipmapsReady() {
    return mipmaps_ready_;
  }

  void SetMipmapsReady(bool ready) {
    mipmaps_ready_ = ready;
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
      mipmaps_ready_ = o.mipmaps_ready_ ? true : false;
    }
    return *this;
  }

  void Reset() {
    type = TextureType_NONE;
    buffer = nullptr;
    mipmaps.clear();
    mipmaps_ready_ = false;
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
  TextureType type = TextureType_NONE;
  std::shared_ptr<Buffer<Vec4<T>>> buffer = nullptr;
  std::vector<std::shared_ptr<Buffer<Vec4<T>>>> mipmaps;

 private:
  std::mutex mutex_;
  std::atomic<bool> mipmaps_ready_{false};
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
class BaseSampler {
 public:
  virtual bool Empty() = 0;
  inline size_t Width() const { return width_; }
  inline size_t Height() const { return height_; }
  inline void SetLodFunc(std::function<float(BaseSampler<T> &)> *func) { lod_func_ = func; }

  Vec4<T> TextureImpl(BaseTexture<T> *tex, glm::vec2 &uv, float lod = 0.f, glm::ivec2 offset = glm::ivec2(0));
  static Vec4<T> SampleNearest(Buffer<Vec4<T>> *buffer, glm::vec2 &uv, WrapMode wrap, glm::ivec2 &offset);
  static Vec4<T> SampleBilinear(Buffer<Vec4<T>> *buffer, glm::vec2 &uv, WrapMode wrap, glm::ivec2 &offset);

  static Vec4<T> PixelWithWrapMode(Buffer<Vec4<T>> *buffer, int x, int y, WrapMode wrap);
  static void SampleBufferBilinear(Buffer<Vec4<T>> *buffer_out, Buffer<Vec4<T>> *buffer_in);
  static Vec4<T> SamplePixelBilinear(Buffer<Vec4<T>> *buffer, glm::vec2 uv, WrapMode wrap);

  inline void SetWrapMode(int wrap_mode) {
    wrap_mode_ = (WrapMode) wrap_mode;
  }

  inline void SetFilterMode(int filter_mode) {
    filter_mode_ = (FilterMode) filter_mode;
  }

 public:
  static const Vec4<T> BORDER_COLOR;

 protected:
  std::function<float(BaseSampler<T> &)> *lod_func_ = nullptr;
  bool use_mipmaps = false;
  size_t width_ = 0;
  size_t height_ = 0;
  WrapMode wrap_mode_ = Wrap_REPEAT;
  FilterMode filter_mode_ = Filter_LINEAR;
};

template<typename T>
class BaseSampler2D : public BaseSampler<T> {
 public:
  inline void SetTexture(BaseTexture<T> *tex) {
    tex_ = tex;
    BaseSampler<T>::width_ = (tex_ == nullptr || tex_->buffer->Empty()) ? 0 : tex_->buffer->GetWidth();
    BaseSampler<T>::height_ = (tex_ == nullptr || tex_->buffer->Empty()) ? 0 : tex_->buffer->GetHeight();
    BaseSampler<T>::use_mipmaps =
        (BaseSampler<T>::filter_mode_ != Filter_NEAREST && BaseSampler<T>::filter_mode_ != Filter_LINEAR);
  }

  inline bool Empty() override {
    return tex_ == nullptr;
  }

  virtual glm::vec4 texture2DImpl(glm::vec2 &uv, float bias = 0.f) {
    float lod = bias;
    if (BaseSampler<T>::use_mipmaps && BaseSampler<T>::lod_func_) {
      lod += (*BaseSampler<T>::lod_func_)(*this);
    }
    return texture2DLodImpl(uv, lod);
  }

  virtual glm::vec4 texture2DLodImpl(glm::vec2 &uv, float lod = 0.f, glm::ivec2 offset = glm::ivec2(0)) {
    if (tex_ == nullptr) {
      return {0, 0, 0, 0};
    }
    Vec4<T> color = BaseSampler<T>::TextureImpl(tex_, uv, lod, offset);
    return {color[0], color[1], color[2], color[3]};
  }

 private:
  BaseTexture<T> *tex_ = nullptr;
};

template<typename T>
class BaseSamplerCube : public BaseSampler<T> {
 public:
  BaseSamplerCube() {
    BaseSampler<T>::wrap_mode_ = Wrap_CLAMP_TO_EDGE;
    BaseSampler<T>::filter_mode_ = Filter_LINEAR;
  }

  inline void SetTexture(BaseTexture<T> *tex, int idx) {
    texes_[idx] = tex;
    if (idx == 0) {
      BaseSampler<T>::width_ = (tex == nullptr || tex->buffer->Empty()) ? 0 : tex->buffer->GetWidth();
      BaseSampler<T>::height_ = (tex == nullptr || tex->buffer->Empty()) ? 0 : tex->buffer->GetHeight();
      BaseSampler<T>::use_mipmaps =
          (BaseSampler<T>::filter_mode_ != Filter_NEAREST && BaseSampler<T>::filter_mode_ != Filter_LINEAR);
    }
  }

  inline bool Empty() override {
    return texes_[0] == nullptr;
  }

  virtual glm::vec4 textureCubeImpl(glm::vec3 &coord, float bias = 0.f) {
    float lod = bias;
    if (BaseSampler<T>::use_mipmaps && BaseSampler<T>::lod_func_) {
      lod += (*BaseSampler<T>::lod_func_)(*this);
    }
    return textureCubeLodImpl(coord, lod);
  }

  virtual glm::vec4 textureCubeLodImpl(glm::vec3 &coord, float lod = 0.f) {
    int index;
    glm::vec2 uv;
    ConvertXYZ2UV(coord.x, coord.y, coord.z, &index, &uv.x, &uv.y);
    BaseTexture<T> *tex = texes_[index];
    if (tex == nullptr) {
      return {0, 0, 0, 0};
    }
    Vec4<T> color = BaseSampler<T>::TextureImpl(tex, uv, lod);
    return {color[0], color[1], color[2], color[3]};
  }

  static void ConvertXYZ2UV(float x, float y, float z, int *index, float *u, float *v);

 private:
  // +x, -x, +y, -y, +z, -z
  BaseTexture<T> *texes_[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
};

template<typename T>
const Vec4<T> BaseSampler<T>::BORDER_COLOR(0);

template<typename T>
void BaseTexture<T>::GenerateMipmaps() {
  std::lock_guard<std::mutex> lock_guard(mutex_);
  if (mipmaps_ready_) {
    return;
  }
  mipmaps.clear();

  int origin_width = (int) buffer->GetWidth();
  int origin_height = (int) buffer->GetHeight();

  // level 0
  int level_size = std::max(RoundupPowerOf2(origin_width), RoundupPowerOf2(origin_height));
  mipmaps.push_back(BaseTexture<T>::CreateBuffer(buffer->GetLayout()));
  Buffer<Vec4<T>> *level_buffer = mipmaps.back().get();
  Buffer<Vec4<T>> *level_buffer_from = buffer.get();
  level_buffer->Create(level_size, level_size);

  if (level_size == level_buffer_from->GetWidth() && level_size == level_buffer_from->GetHeight()) {
    level_buffer_from->CopyRawDataTo(level_buffer->GetRawDataPtr());
  } else {
    BaseSampler<T>::SampleBufferBilinear(level_buffer, level_buffer_from);
  }

  // other levels
  while (level_size >= 2) {
    level_size /= 2;

    mipmaps.push_back(BaseTexture<T>::CreateBuffer(buffer->GetLayout()));
    level_buffer = mipmaps.back().get();
    level_buffer_from = mipmaps[mipmaps.size() - 2].get();
    level_buffer->Create(level_size, level_size);

    BaseSampler<T>::SampleBufferBilinear(level_buffer, level_buffer_from);
  }

  mipmaps_ready_ = true;
}

template<typename T>
Vec4<T> BaseSampler<T>::TextureImpl(BaseTexture<T> *tex, glm::vec2 &uv, float lod, glm::ivec2 offset) {
  if (tex != nullptr && !tex->buffer->Empty()) {
    if (filter_mode_ == Filter_NEAREST) {
      return SampleNearest(tex->buffer.get(), uv, wrap_mode_, offset);
    }
    if (filter_mode_ == Filter_LINEAR) {
      return SampleBilinear(tex->buffer.get(), uv, wrap_mode_, offset);
    }

    // mipmaps not ready
    if (!tex->MipmapsReady()) {
      tex->GenerateMipmaps();
      if (filter_mode_ == Filter_NEAREST_MIPMAP_NEAREST || filter_mode_ == Filter_NEAREST_MIPMAP_LINEAR) {
        return SampleNearest(tex->buffer.get(), uv, wrap_mode_, offset);
      }
      if (filter_mode_ == Filter_LINEAR_MIPMAP_NEAREST || filter_mode_ == Filter_LINEAR_MIPMAP_LINEAR) {
        return SampleBilinear(tex->buffer.get(), uv, wrap_mode_, offset);
      }
    }

    // mipmaps ready
    int max_level = (int) tex->mipmaps.size() - 1;

    if (filter_mode_ == Filter_NEAREST_MIPMAP_NEAREST || filter_mode_ == Filter_LINEAR_MIPMAP_NEAREST) {
      int level = glm::clamp((int) std::round(lod), 0, max_level);
      if (filter_mode_ == Filter_NEAREST_MIPMAP_NEAREST) {
        return SampleNearest(tex->mipmaps[level].get(), uv, wrap_mode_, offset);
      } else {
        return SampleBilinear(tex->mipmaps[level].get(), uv, wrap_mode_, offset);
      }
    }

    if (filter_mode_ == Filter_NEAREST_MIPMAP_LINEAR || filter_mode_ == Filter_LINEAR_MIPMAP_LINEAR) {
      int level1 = glm::clamp((int) std::floor(lod), 0, max_level);
      int level2 = glm::clamp((int) std::ceil(lod), 0, max_level);

      Vec4<T> texel1, texel2;
      if (filter_mode_ == Filter_NEAREST_MIPMAP_LINEAR) {
        texel1 = SampleNearest(tex->mipmaps[level1].get(), uv, wrap_mode_, offset);
      } else {
        texel1 = SampleBilinear(tex->mipmaps[level1].get(), uv, wrap_mode_, offset);
      }

      if (level1 == level2) {
        return texel1;
      } else {
        if (filter_mode_ == Filter_NEAREST_MIPMAP_LINEAR) {
          texel2 = SampleNearest(tex->mipmaps[level2].get(), uv, wrap_mode_, offset);
        } else {
          texel2 = SampleBilinear(tex->mipmaps[level2].get(), uv, wrap_mode_, offset);
        }
      }

      float f = glm::fract(lod);
      return glm::mix(texel1, texel2, f);
    }
  }
  return Vec4<T>(0);
}

template<typename T>
Vec4<T> BaseSampler<T>::PixelWithWrapMode(Buffer<Vec4<T>> *buffer, int x, int y, WrapMode wrap) {
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
      if (x < 0 || x >= w) return Vec4<T>(0);
      if (y < 0 || y >= h) return Vec4<T>(0);
    }
      break;
  }

  Vec4<T> *ptr = buffer->Get(x, y);
  if (ptr) {
    return *ptr;
  }
  return Vec4<T>(0);
}

template<typename T>
Vec4<T> BaseSampler<T>::SampleNearest(Buffer<Vec4<T>> *buffer, glm::vec2 &uv, WrapMode wrap, glm::ivec2 &offset) {
  glm::vec2 texUV = uv * glm::vec2(buffer->GetWidth(), buffer->GetHeight());
  auto x = (int) texUV.x + offset.x;
  auto y = (int) texUV.y + offset.y;

  return PixelWithWrapMode(buffer, x, y, wrap);
}

template<typename T>
Vec4<T> BaseSampler<T>::SampleBilinear(Buffer<Vec4<T>> *buffer, glm::vec2 &uv, WrapMode wrap, glm::ivec2 &offset) {
  glm::vec2 texUV = uv * glm::vec2(buffer->GetWidth(), buffer->GetHeight()) - glm::vec2(0.5f);
  texUV.x += offset.x;
  texUV.y += offset.y;
  return SamplePixelBilinear(buffer, texUV, wrap);
}

template<typename T>
void BaseSampler<T>::SampleBufferBilinear(Buffer<Vec4<T>> *buffer_out, Buffer<Vec4<T>> *buffer_in) {
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
Vec4<T> BaseSampler<T>::SamplePixelBilinear(Buffer<Vec4<T>> *buffer, glm::vec2 uv, WrapMode wrap) {
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

  // Convert range from -1 to 1 to 0 to 1
  *u = 0.5f * (uc / maxAxis + 1.0f);
  *v = 0.5f * (vc / maxAxis + 1.0f);
}

class Sampler2D : public BaseSampler2D<float > {
 public:

  glm::vec4 texture2D(glm::vec2 uv, float bias = 0.f) {
    return BaseSampler2D::texture2DImpl(uv, bias);
  }

  glm::vec4 texture2DLod(glm::vec2 uv, float lod = 0.f) {
    return BaseSampler2D::texture2DLodImpl(uv, lod);
  }

  glm::vec4 texture2DLodOffset(glm::vec2 uv, float lod, glm::ivec2 offset) {
    return BaseSampler2D::texture2DLodImpl(uv, lod, offset);
  }
};

class SamplerCube : public BaseSamplerCube<float> {
 public:

  glm::vec4 textureCube(glm::vec3 coord, float bias = 0.f) {
    return BaseSamplerCube::textureCubeImpl(coord, bias);
  }

  glm::vec4 textureCubeLod(glm::vec3 coord, float lod = 0.f) {
    return BaseSamplerCube::textureCubeLodImpl(coord, lod);
  }
};

using Texture = BaseTexture<float>;
using Sampler = BaseSampler<float>;

}
