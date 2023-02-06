/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/buffer.h"
#include "render/texture.h"

namespace SoftGL {

class Texture2DSoft : public Texture2D {
 public:
  Texture2DSoft() : uuid_(uuid_counter_++) {
  }

  ~Texture2DSoft() = default;

  int GetId() const override {
    return uuid_;
  }

  std::shared_ptr<BufferRGBA> GetBuffer() {
    return buffer_;
  }

  void SetSamplerDesc(SamplerDesc &sampler) override {
  }

  void SetImageData(const std::vector<std::shared_ptr<BufferRGBA>> &buffers) override {
    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();
    buffer_ = buffers[0];
  }

  void InitImageData(int w, int h) override {
    if (width == w && height == h) {
      return;
    }
    width = w;
    height = h;
    if (!buffer_) {
      buffer_ = BufferRGBA::MakeDefault();
    }
    buffer_->Create(w, h);
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  std::shared_ptr<BufferRGBA> buffer_ = nullptr;
};

class TextureCubeSoft : public TextureCube {
 public:
  TextureCubeSoft() : uuid_(uuid_counter_++) {
  }

  ~TextureCubeSoft() = default;

  int GetId() const override {
    return uuid_;
  }

  std::shared_ptr<BufferRGBA> GetBuffer(CubeMapFace face) {
    return buffers_[face];
  }

  void SetSamplerDesc(SamplerDesc &sampler) override {
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

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  std::shared_ptr<BufferRGBA> buffers_[6];
};

class TextureDepthSoft : public TextureDepth {
 public:
  TextureDepthSoft() : uuid_(uuid_counter_++) {
  }

  ~TextureDepthSoft() = default;

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