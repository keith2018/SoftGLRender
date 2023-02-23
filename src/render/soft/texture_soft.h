/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <thread>
#include <atomic>
#include "base/buffer.h"
#include "render/texture.h"

namespace SoftGL {

template<typename T>
class TextureImageSoft {
 public:
  std::vector<std::shared_ptr<Buffer<glm::tvec4<T>>>> levels;

 public:
  inline size_t GetWidth() {
    return Empty() ? 0 : levels[0]->GetWidth();
  }

  inline size_t GetHeight() {
    return Empty() ? 0 : levels[0]->GetHeight();
  }

  inline bool Empty() {
    return levels.empty();
  }

  inline std::shared_ptr<Buffer<glm::tvec4<T>>> &GetBuffer(int level = 0) {
    return levels[level];
  }

  void GenerateMipmap(bool sample = true);
};

class Texture2DSoft : public Texture2D {
 public:
  explicit Texture2DSoft(bool multi_sample = false) : uuid_(uuid_counter_++) {
    Texture::multi_sample = multi_sample;
  }

  int GetId() const override {
    return uuid_;
  }

  void SetSamplerDesc(SamplerDesc &sampler) override {
    sampler_desc_ = dynamic_cast<Sampler2DDesc &>(sampler);
  }

  void SetImageData(const std::vector<std::shared_ptr<BufferRGBA>> &buffers) override {
    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();

    image_.levels.resize(1);
    image_.levels[0] = buffers[0];

    if (sampler_desc_.use_mipmaps) {
      image_.GenerateMipmap();
    }
  }

  void InitImageData(int w, int h) override {
    if (width == w && height == h) {
      return;
    }
    width = w;
    height = h;

    image_.levels.resize(1);
    image_.levels[0] = BufferRGBA::MakeDefault(w, h);

    if (sampler_desc_.use_mipmaps) {
      image_.GenerateMipmap(false);
    }
  }

  inline Sampler2DDesc &GetSamplerDesc() {
    return sampler_desc_;
  }

  inline TextureImageSoft<uint8_t> &GetImage() {
    return image_;
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  Sampler2DDesc sampler_desc_;
  TextureImageSoft<uint8_t> image_;
};

class TextureCubeSoft : public TextureCube {
 public:
  TextureCubeSoft() : uuid_(uuid_counter_++) {}

  int GetId() const override {
    return uuid_;
  }

  void SetSamplerDesc(SamplerDesc &sampler) override {
    sampler_desc_ = dynamic_cast<SamplerCubeDesc &>(sampler);
  }

  void SetImageData(const std::vector<std::shared_ptr<BufferRGBA>> &buffers) override {
    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();
    for (int i = 0; i < 6; i++) {
      images_[i].levels.resize(1);
      images_[i].levels[0] = buffers[i];

      if (sampler_desc_.use_mipmaps) {
        images_[i].GenerateMipmap();
      }
    }
  }

  void InitImageData(int w, int h) override {
    if (width == w && height == h) {
      return;
    }
    width = w;
    height = h;

    for (auto &image : images_) {
      image.levels.resize(1);
      image.levels[0] = BufferRGBA::MakeDefault(w, h);

      if (sampler_desc_.use_mipmaps) {
        image.GenerateMipmap(false);
      }
    }
  }

  inline SamplerCubeDesc &GetSamplerDesc() {
    return sampler_desc_;
  }

  inline TextureImageSoft<uint8_t> &GetImage(CubeMapFace face) {
    return images_[face];
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  SamplerCubeDesc sampler_desc_;
  TextureImageSoft<uint8_t> images_[6];
};

class BufferDepth {
 public:
  BufferDepth(int w, int h, bool multi_sample) {
    width = w;
    height = h;
    sample_cnt = multi_sample ? 4 : 1;
    if (multi_sample) {
      buffer = Buffer<float>::MakeDefault(w, h);
    } else {
      buffer_ms4x = Buffer<glm::fvec4>::MakeDefault(w, h);
    }
  }

  inline void SetAll(float val) {
    if (buffer) {
      buffer->SetAll(val);
    } else {
      buffer_ms4x->SetAll(glm::fvec4(val));
    }
  }

  inline float *Get(size_t x, size_t y, int sample = 0) {
    if (buffer) {
      return buffer->Get(x, y);
    } else {
      auto *ptr = (float *) buffer_ms4x->Get(x, y);
      if (ptr) {
        return ptr + sample;
      }
    }

    return nullptr;
  }

 public:
  std::shared_ptr<Buffer<float>> buffer;
  std::shared_ptr<Buffer<glm::fvec4>> buffer_ms4x;

  int width = 0;
  int height = 0;
  int sample_cnt = 0;
};

class TextureDepthSoft : public TextureDepth {
 public:
  explicit TextureDepthSoft(bool multi_sample = false) : uuid_(uuid_counter_++) {
    Texture::multi_sample = multi_sample;
  }

  int GetId() const override {
    return uuid_;
  }

  inline std::shared_ptr<BufferDepth> &GetBuffer() {
    return image_;
  }

  void InitImageData(int w, int h) override {
    if (width == w && height == h) {
      return;
    }
    width = w;
    height = h;

    image_ = std::make_shared<BufferDepth>(w, h, multi_sample);
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  std::shared_ptr<BufferDepth> image_;
};

}
