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
  std::vector<std::shared_ptr<Buffer<T>>> levels;

  std::atomic<bool> has_content = {false};
  std::atomic<bool> mipmaps_ready = {false};
  std::atomic<bool> mipmaps_generating = {false};
  std::shared_ptr<std::thread> mipmaps_thread = nullptr;

  std::function<void(TextureImageSoft &)> mipmaps_func;  // TODO

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

  inline std::shared_ptr<Buffer<T>> &GetBuffer(int level = 0) {
    return levels[level];
  }

  inline void GenerateMipmap() {
    if (mipmaps_func) {
      mipmaps_func(*this);
    }
  }

  virtual ~TextureImageSoft() {
    if (mipmaps_thread) {
      mipmaps_thread->join();
    }
  };
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

    image_.levels.resize(1);
    image_.levels[0] = buffers[0];
    image_.has_content = true;

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
    image_.levels[0] = BufferRGBA::MakeDefault();
    image_.levels[0]->Create(w, h);
    image_.has_content = false;

    if (sampler_desc_.use_mipmaps) {
      image_.GenerateMipmap();
    }
  }

  inline std::shared_ptr<BufferRGBA> GetBuffer(int level = 0) const {
    return image_.levels[level];
  }

  inline Sampler2DDesc &GetSamplerDesc() {
    return sampler_desc_;
  }

  inline TextureImageSoft<glm::tvec4<uint8_t>> &GetImage() {
    return image_;
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  Sampler2DDesc sampler_desc_;
  TextureImageSoft<glm::tvec4<uint8_t>> image_;
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
      images_[i].has_content = true;

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
      image.levels[0] = BufferRGBA::MakeDefault();
      image.levels[0]->Create(w, h);
      image.has_content = false;

      if (sampler_desc_.use_mipmaps) {
        image.GenerateMipmap();
      }
    }
  }

  std::shared_ptr<BufferRGBA> GetBuffer(CubeMapFace face, int level = 0) {
    return images_[face].levels[level];
  }

  inline SamplerCubeDesc &GetSamplerDesc() {
    return sampler_desc_;
  }

  inline TextureImageSoft<glm::tvec4<uint8_t>> &GetImage(CubeMapFace face) {
    return images_[face];
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  SamplerCubeDesc sampler_desc_;
  TextureImageSoft<glm::tvec4<uint8_t>> images_[6];
};

class TextureDepthSoft : public TextureDepth {
 public:
  TextureDepthSoft() : uuid_(uuid_counter_++) {}

  int GetId() const override {
    return uuid_;
  }

  std::shared_ptr<BufferDepth> GetBuffer() const {
    return image_.levels[0];
  }

  inline TextureImageSoft<float> &GetImage() {
    return image_;
  }

  void InitImageData(int w, int h) override {
    if (width == w && height == h) {
      return;
    }
    width = w;
    height = h;

    image_.levels.resize(1);
    image_.levels[0] = BufferDepth::MakeDefault();
    image_.levels[0]->Create(w, h);
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
  TextureImageSoft<float> image_;
};

}
