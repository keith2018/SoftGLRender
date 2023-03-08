/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <thread>
#include <atomic>
#include "base/buffer.h"
#include "base/image_utils.h"
#include "render/texture.h"

namespace SoftGL {

#define SOFT_MS_CNT 4

template<typename T>
class ImageBufferSoft {
 public:
  ImageBufferSoft() = default;

  ImageBufferSoft(int w, int h, int samples = 1) {
    width = w;
    height = h;
    multi_sample = samples > 1;
    sample_cnt = samples;

    if (samples == 1) {
      buffer = Buffer<T>::MakeDefault(w, h);
    } else if (samples == 4) {
      buffer_ms4x = Buffer<glm::tvec4<T>>::MakeDefault(w, h);
    } else {
      LOGE("create color buffer failed: samplers not support");
    }
  }

  explicit ImageBufferSoft(const std::shared_ptr<Buffer<T>> &buf) {
    width = (int) buf->GetWidth();
    height = (int) buf->GetHeight();
    multi_sample = false;
    sample_cnt = 1;
    buffer = buf;
  }

 public:
  std::shared_ptr<Buffer<T>> buffer;
  std::shared_ptr<Buffer<glm::tvec4<T>>> buffer_ms4x;

  int width = 0;
  int height = 0;
  bool multi_sample = false;
  int sample_cnt = 1;
};

template<typename T>
class TextureImageSoft {
 public:
  inline int GetWidth() {
    return Empty() ? 0 : levels[0]->width;
  }

  inline int GetHeight() {
    return Empty() ? 0 : levels[0]->height;
  }

  inline bool Empty() const {
    return levels.empty();
  }

  inline std::shared_ptr<ImageBufferSoft<T>> &GetBuffer(int level = 0) {
    return levels[level];
  }

  void GenerateMipmap(bool sample = true);

 public:
  std::vector<std::shared_ptr<ImageBufferSoft<T>>> levels;
};

template<typename T>
class Texture2DSoft : public Texture {
 public:
  explicit Texture2DSoft(const TextureDesc &desc) : uuid_(uuid_counter_++) {
    assert(desc.type == TextureType_2D);

    type = TextureType_2D;
    format = desc.format;
    multi_sample = desc.multi_sample;
  }

  int GetId() const override {
    return uuid_;
  }

  void SetSamplerDesc(SamplerDesc &sampler) override {
    sampler_desc_ = dynamic_cast<Sampler2DDesc &>(sampler);
  }

  void SetImageData(const std::vector<std::shared_ptr<Buffer<T>>> &buffers) override {
    if (multi_sample) {
      LOGE("SetImageData not support: multi sample texture");
      return;
    }

    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();

    image_.levels.resize(1);
    image_.levels[0] = std::make_shared<ImageBufferSoft<T>>(buffers[0]);

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
    image_.levels[0] = std::make_shared<ImageBufferSoft<T>>(w, h, multi_sample ? SOFT_MS_CNT : 1);
    if (sampler_desc_.use_mipmaps) {
      image_.GenerateMipmap(false);
    }
  }

  void DumpImage(const char *path) override {
    if (multi_sample) {
      return;
    }

    void *pixels = image_.GetBuffer()->buffer->GetRawDataPtr();

    // convert float to rgba
    if (format == TextureFormat_DEPTH) {
      auto *rgba_pixels = new uint8_t[width * height * 4];
      ImageUtils::ConvertFloatImage(reinterpret_cast<RGBA *>(rgba_pixels),
                                    reinterpret_cast<float *>(pixels),
                                    width,
                                    height);
      ImageUtils::WriteImage(path, width, height, 4, rgba_pixels, width * 4, true);
      delete[] rgba_pixels;
    } else {
      ImageUtils::WriteImage(path, width, height, 4, pixels, width * 4, true);
    }
  }

  inline Sampler2DDesc &GetSamplerDesc() {
    return sampler_desc_;
  }

  inline TextureImageSoft<T> &GetImage() {
    return image_;
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  Sampler2DDesc sampler_desc_;
  TextureImageSoft<T> image_;
};

template<typename T>
class TextureCubeSoft : public Texture {
 public:
  explicit TextureCubeSoft(const TextureDesc &desc) : uuid_(uuid_counter_++) {
    assert(desc.type == TextureType_CUBE);
    assert(desc.multi_sample == false);

    type = TextureType_CUBE;
    format = desc.format;
    multi_sample = desc.multi_sample;
  }

  int GetId() const override {
    return uuid_;
  }

  void SetSamplerDesc(SamplerDesc &sampler) override {
    sampler_desc_ = dynamic_cast<SamplerCubeDesc &>(sampler);
  }

  void SetImageData(const std::vector<std::shared_ptr<Buffer<T>>> &buffers) override {
    if (multi_sample) {
      LOGE("SetImageData not support: multi sample texture");
      return;
    }

    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();

    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();

    for (int i = 0; i < 6; i++) {
      images_[i].levels.resize(1);
      images_[i].levels[0] = std::make_shared<ImageBufferSoft<T>>(buffers[i]);

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
      image.levels[0] = std::make_shared<ImageBufferSoft<T>>(w, h);
      if (sampler_desc_.use_mipmaps) {
        image.GenerateMipmap(false);
      }
    }
  }

  inline SamplerCubeDesc &GetSamplerDesc() {
    return sampler_desc_;
  }

  inline TextureImageSoft<T> &GetImage(CubeMapFace face) {
    return images_[face];
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  SamplerCubeDesc sampler_desc_;
  TextureImageSoft<T> images_[6];
};

template<typename T>
int Texture2DSoft<T>::uuid_counter_ = 0;

template<typename T>
int TextureCubeSoft<T>::uuid_counter_ = 0;

}
