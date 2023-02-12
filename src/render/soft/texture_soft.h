/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <thread>
#include "base/buffer.h"
#include "render/texture.h"

namespace SoftGL {

template<typename T>
class TextureImageSoft {
 public:
  std::shared_ptr<Buffer<glm::tvec4<T>>> buffer = nullptr;
};

class Texture2DSoft : public Texture2D {
 public:
  Texture2DSoft() : uuid_(uuid_counter_++) {}

  int GetId() const override {
    return uuid_;
  }

  void SetSamplerDesc(SamplerDesc &sampler) override {
    sampler_desc_ = dynamic_cast<Sampler2DDesc &>(sampler);
  }

  void SetImageData(const std::vector<std::shared_ptr<BufferRGBA>> &buffers) override {
    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();

    image_.buffer = buffers[0];
  }

  void InitImageData(int w, int h) override {
    if (width == w && height == h) {
      return;
    }
    width = w;
    height = h;

    if (!image_.buffer) {
      image_.buffer = BufferRGBA::MakeDefault();
    }
    image_.buffer->Create(w, h);
  }

  inline std::shared_ptr<BufferRGBA> GetBuffer() const {
    return image_.buffer;
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

  std::shared_ptr<BufferRGBA> GetBuffer(CubeMapFace face) {
    return buffers_[face];
  }

  void SetSamplerDesc(SamplerDesc &sampler) override {
    sampler_desc_ = dynamic_cast<SamplerCubeDesc &>(sampler);
  }

  void SetImageData(const std::vector<std::shared_ptr<BufferRGBA>> &buffers) override {
    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();
    for (int i = 0; i < 6; i++) {
      buffers_[i] = buffers[i];
    }
  }

  void InitImageData(int w, int h) override {
    if (width == w && height == h) {
      return;
    }
    width = w;
    height = h;
    for (int i = 0; i < 6; i++) {
      if (!buffers_[i]) {
        buffers_[i] = BufferRGBA::MakeDefault();
      }
      buffers_[i]->Create(w, h);
    }
  }

  inline SamplerCubeDesc &GetSamplerDesc() {
    return sampler_desc_;
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  SamplerCubeDesc sampler_desc_;
  std::shared_ptr<BufferRGBA> buffers_[6];
};

class TextureDepthSoft : public TextureDepth {
 public:
  TextureDepthSoft() : uuid_(uuid_counter_++) {}

  int GetId() const override {
    return uuid_;
  }

  std::shared_ptr<BufferDepth> GetBuffer() {
    return buffer_;
  }

  void InitImageData(int w, int h) override {
    if (width == w && height == h) {
      return;
    }
    width = w;
    height = h;
    if (!buffer_) {
      buffer_ = BufferDepth::MakeDefault();
    }
    buffer_->Create(w, h);
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  std::shared_ptr<BufferDepth> buffer_ = nullptr;
};

}
