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

#define SOFT_MULTI_SAMPLE_CNT 4

class ImageBufferColor {
 public:
  ImageBufferColor() = default;

  ImageBufferColor(int w, int h, int samples = 1) {
    width = w;
    height = h;
    multi_sample = samples > 1;
    sample_cnt = samples;

    if (samples == 1) {
      buffer = Buffer<RGBA>::MakeDefault(w, h);
    } else if (samples == 4) {
      buffer_ms4x = Buffer<glm::tvec4<RGBA>>::MakeDefault(w, h);
    } else {
      LOGE("create color buffer failed: samplers not support");
    }
  }

  explicit ImageBufferColor(const std::shared_ptr<Buffer<RGBA>> &buf) {
    width = (int) buf->GetWidth();
    height = (int) buf->GetHeight();
    multi_sample = false;
    sample_cnt = 1;
    buffer = buf;
  }

 public:
  std::shared_ptr<Buffer<RGBA>> buffer;
  std::shared_ptr<Buffer<glm::tvec4<RGBA>>> buffer_ms4x;

  int width = 0;
  int height = 0;
  bool multi_sample = false;
  int sample_cnt = 1;
};

class ImageBufferDepth {
 public:
  ImageBufferDepth() = default;

  ImageBufferDepth(int w, int h, int samples = 1) {
    width = w;
    height = h;
    multi_sample = samples > 1;
    sample_cnt = samples;

    if (samples == 1) {
      buffer = Buffer<float>::MakeDefault(w, h);
    } else if (samples == 4) {
      buffer_ms4x = Buffer<glm::tvec4<float>>::MakeDefault(w, h);
    } else {
      LOGE("create depth buffer failed: samplers not support");
    }
  }

 public:
  std::shared_ptr<Buffer<float>> buffer;
  std::shared_ptr<Buffer<glm::tvec4<float>>> buffer_ms4x;

  int width = 0;
  int height = 0;
  bool multi_sample = false;
  int sample_cnt = 1;
};

class TextureImageSoft {
 public:
  std::vector<std::shared_ptr<ImageBufferColor>> levels;

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

  inline std::shared_ptr<ImageBufferColor> &GetBuffer(int level = 0) {
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

  void SetImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) override {
    if (multi_sample) {
      LOGE("set image data not support: multi sample texture");
      return;
    }

    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();

    image_.levels.resize(1);
    image_.levels[0] = std::make_shared<ImageBufferColor>(buffers[0]);

    if (!multi_sample && sampler_desc_.use_mipmaps) {
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
    image_.levels[0] = std::make_shared<ImageBufferColor>(w, h,
                                                          multi_sample ? SOFT_MULTI_SAMPLE_CNT : 1);

    if (!multi_sample && sampler_desc_.use_mipmaps) {
      image_.GenerateMipmap(false);
    }
  }

  inline Sampler2DDesc &GetSamplerDesc() {
    return sampler_desc_;
  }

  inline TextureImageSoft &GetImage() {
    return image_;
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  Sampler2DDesc sampler_desc_;
  TextureImageSoft image_;
};

class TextureCubeSoft : public TextureCube {
 public:
  TextureCubeSoft() : uuid_(uuid_counter_++) {
    Texture::multi_sample = false;
  }

  int GetId() const override {
    return uuid_;
  }

  void SetSamplerDesc(SamplerDesc &sampler) override {
    sampler_desc_ = dynamic_cast<SamplerCubeDesc &>(sampler);
  }

  void SetImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) override {
    if (multi_sample) {
      LOGE("set image data not support: multi sample texture");
      return;
    }

    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();
    for (int i = 0; i < 6; i++) {
      images_[i].levels.resize(1);
      images_[i].levels[0] = std::make_shared<ImageBufferColor>(buffers[i]);

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
      image.levels[0] = std::make_shared<ImageBufferColor>(w, h);

      if (sampler_desc_.use_mipmaps) {
        image.GenerateMipmap(false);
      }
    }
  }

  inline SamplerCubeDesc &GetSamplerDesc() {
    return sampler_desc_;
  }

  inline TextureImageSoft &GetImage(CubeMapFace face) {
    return images_[face];
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  SamplerCubeDesc sampler_desc_;
  TextureImageSoft images_[6];
};

class TextureDepthSoft : public TextureDepth {
 public:
  explicit TextureDepthSoft(bool multi_sample = false) : uuid_(uuid_counter_++) {
    Texture::multi_sample = multi_sample;
  }

  int GetId() const override {
    return uuid_;
  }

  inline std::shared_ptr<ImageBufferDepth> &GetBuffer() {
    return image_;
  }

  void InitImageData(int w, int h) override {
    if (width == w && height == h) {
      return;
    }
    width = w;
    height = h;

    image_ = std::make_shared<ImageBufferDepth>(w, h,
                                                multi_sample ? SOFT_MULTI_SAMPLE_CNT : 1);
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  std::shared_ptr<ImageBufferDepth> image_;
};

}
