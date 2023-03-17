/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <thread>
#include <atomic>
#include "Base/UUID.h"
#include "Base/Buffer.h"
#include "Base/ImageUtils.h"
#include "Render/Texture.h"

namespace SoftGL {

#define SOFT_MS_CNT 4

template<typename T>
class ImageBufferSoft {
 public:
  ImageBufferSoft() = default;

  ImageBufferSoft(int w, int h, int samples = 1) {
    width = w;
    height = h;
    multiSample = samples > 1;
    sampleCnt = samples;

    if (samples == 1) {
      buffer = Buffer<T>::makeDefault(w, h);
    } else if (samples == 4) {
      bufferMs4x = Buffer<glm::tvec4<T>>::makeDefault(w, h);
    } else {
      LOGE("create color buffer failed: samplers not support");
    }
  }

  explicit ImageBufferSoft(const std::shared_ptr<Buffer<T>> &buf) {
    width = (int) buf->getWidth();
    height = (int) buf->getHeight();
    multiSample = false;
    sampleCnt = 1;
    buffer = buf;
  }

 public:
  std::shared_ptr<Buffer<T>> buffer;
  std::shared_ptr<Buffer<glm::tvec4<T>>> bufferMs4x;

  int width = 0;
  int height = 0;
  bool multiSample = false;
  int sampleCnt = 1;
};

template<typename T>
class TextureImageSoft {
 public:
  inline int getWidth() {
    return empty() ? 0 : levels[0]->width;
  }

  inline int getHeight() {
    return empty() ? 0 : levels[0]->height;
  }

  inline bool empty() const {
    return levels.empty();
  }

  inline std::shared_ptr<ImageBufferSoft<T>> &getBuffer(int level = 0) {
    return levels[level];
  }

  void generateMipmap(bool sample = true);

 public:
  std::vector<std::shared_ptr<ImageBufferSoft<T>>> levels;
};

template<typename T>
class Texture2DSoft : public Texture {
 public:
  explicit Texture2DSoft(const TextureDesc &desc) {
    assert(desc.type == TextureType_2D);

    width = desc.width;
    height = desc.height;
    type = TextureType_2D;
    format = desc.format;
    multiSample = desc.multiSample;
  }

  int getId() const override {
    return uuid_.get();
  }

  void setSamplerDesc(SamplerDesc &sampler) override {
    samplerDesc_ = dynamic_cast<Sampler2DDesc &>(sampler);
  }

  void setImageData(const std::vector<std::shared_ptr<Buffer<T>>> &buffers) override {
    if (multiSample) {
      LOGE("setImageData not support: multi sample texture");
      return;
    }

    if (width != buffers[0]->getWidth() || height != buffers[0]->getHeight()) {
      LOGE("setImageData error: size not match");
      return;
    }

    image_.levels.resize(1);
    image_.levels[0] = std::make_shared<ImageBufferSoft<T>>(buffers[0]);

    if (samplerDesc_.useMipmaps) {
      image_.generateMipmap();
    }
  }

  void initImageData() override {
    image_.levels.resize(1);
    image_.levels[0] = std::make_shared<ImageBufferSoft<T>>(width, height, multiSample ? SOFT_MS_CNT : 1);
    if (samplerDesc_.useMipmaps) {
      image_.generateMipmap(false);
    }
  }

  void dumpImage(const char *path) override {
    if (multiSample) {
      return;
    }

    void *pixels = image_.getBuffer()->buffer->getRawDataPtr();

    // convert float to rgba
    if (format == TextureFormat_FLOAT32) {
      auto *rgba_pixels = new uint8_t[width * height * 4];
      ImageUtils::convertFloatImage(reinterpret_cast<RGBA *>(rgba_pixels), reinterpret_cast<float *>(pixels),
                                    width, height);
      ImageUtils::writeImage(path, width, height, 4, rgba_pixels, width * 4, true);
      delete[] rgba_pixels;
    } else {
      ImageUtils::writeImage(path, width, height, 4, pixels, width * 4, true);
    }
  }

  inline Sampler2DDesc &getSamplerDesc() {
    return samplerDesc_;
  }

  inline TextureImageSoft<T> &getImage() {
    return image_;
  }

  inline void getBorderColor(float &ret) {
    ret = glm::clamp(samplerDesc_.borderColor.r, 0.f, 1.f);
  }

  inline void getBorderColor(RGBA &ret) {
    ret = glm::clamp(samplerDesc_.borderColor * 255.f, {0, 0, 0, 0},
                     {255, 255, 255, 255});
  }

 private:
  UUID<Texture2DSoft<T>> uuid_;
  Sampler2DDesc samplerDesc_;
  TextureImageSoft<T> image_;
};

template<typename T>
class TextureCubeSoft : public Texture {
 public:
  explicit TextureCubeSoft(const TextureDesc &desc) {
    assert(desc.type == TextureType_CUBE);
    assert(desc.multiSample == false);

    width = desc.width;
    height = desc.height;
    type = TextureType_CUBE;
    format = desc.format;
    multiSample = desc.multiSample;
  }

  int getId() const override {
    return uuid_.get();
  }

  void setSamplerDesc(SamplerDesc &sampler) override {
    samplerDesc_ = dynamic_cast<SamplerCubeDesc &>(sampler);
  }

  void setImageData(const std::vector<std::shared_ptr<Buffer<T>>> &buffers) override {
    if (multiSample) {
      LOGE("setImageData not support: multi sample texture");
      return;
    }

    if (width != buffers[0]->getWidth() || height != buffers[0]->getHeight()) {
      LOGE("setImageData error: size not match");
      return;
    }

    for (int i = 0; i < 6; i++) {
      images_[i].levels.resize(1);
      images_[i].levels[0] = std::make_shared<ImageBufferSoft<T>>(buffers[i]);

      if (samplerDesc_.useMipmaps) {
        images_[i].generateMipmap();
      }
    }
  }

  void initImageData() override {
    for (auto &image : images_) {
      image.levels.resize(1);
      image.levels[0] = std::make_shared<ImageBufferSoft<T>>(width, height);
      if (samplerDesc_.useMipmaps) {
        image.generateMipmap(false);
      }
    }
  }

  inline SamplerCubeDesc &getSamplerDesc() {
    return samplerDesc_;
  }

  inline TextureImageSoft<T> &getImage(CubeMapFace face) {
    return images_[face];
  }

  inline void getBorderColor(float &ret) {
    ret = glm::clamp(samplerDesc_.borderColor.r, 0.f, 1.f);
  }

  inline void getBorderColor(RGBA &ret) {
    ret = glm::clamp(samplerDesc_.borderColor * 255.f, {0, 0, 0, 0},
                     {255, 255, 255, 255});
  }

 private:
  UUID<TextureCubeSoft<T>> uuid_;
  SamplerCubeDesc samplerDesc_;
  TextureImageSoft<T> images_[6];
};

}
