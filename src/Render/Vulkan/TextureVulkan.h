/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/UUID.h"
#include "Render/Texture.h"
#include "VulkanUtils.h"

namespace SoftGL {

class TextureVulkan : public Texture {
 public:
  TextureVulkan() {

  }

  int getId() const override {
    return uuid_.get();
  }

  void setImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) override {

  };

  void setImageData(const std::vector<std::shared_ptr<Buffer<float>>> &buffers) override {

  };

  void initImageData() override {

  };
  void dumpImage(const char *path) override {

  };

 private:
  UUID<TextureVulkan> uuid_;
  VkDevice device_ = VK_NULL_HANDLE;
  VkImage image_ = VK_NULL_HANDLE;
  VkDeviceMemory imageMem_ = VK_NULL_HANDLE;
};

class Texture2DVulkan : public TextureVulkan {
 public:
  void setSamplerDesc(SamplerDesc &sampler) override {
    samplerDesc_ = dynamic_cast<Sampler2DDesc &>(sampler);
  };

 private:
  Sampler2DDesc samplerDesc_;
};

class TextureCubeVulkan : public TextureVulkan {
 public:
  void setSamplerDesc(SamplerDesc &sampler) override {
    samplerDesc_ = dynamic_cast<SamplerCubeDesc &>(sampler);
  };

 private:
  SamplerCubeDesc samplerDesc_;
};

}
