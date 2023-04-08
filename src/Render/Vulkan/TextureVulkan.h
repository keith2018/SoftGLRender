/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <functional>
#include "Base/UUID.h"
#include "Base/ImageUtils.h"
#include "Render/Texture.h"
#include "VKContext.h"
#include "EnumsVulkan.h"

namespace SoftGL {

class TextureVulkan : public Texture {
 public:
  TextureVulkan(VKContext &ctx, const TextureDesc &desc);

  ~TextureVulkan() override;

  inline int getId() const override {
    return uuid_.get();
  }

  inline void setSamplerDesc(SamplerDesc &sampler) override {
    samplerDesc_ = sampler;
  };

  void initImageData() override;

  void dumpImage(const char *path, uint32_t w, uint32_t h) override;

  void setImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) override;

  void setImageData(const std::vector<std::shared_ptr<Buffer<float>>> &buffers) override;

  void readPixels(uint32_t layer, uint32_t level,
                  const std::function<void(uint8_t *buffer, uint32_t width, uint32_t height, uint32_t rowStride)> &func);

  VkSampler &getSampler();

  inline VkSampleCountFlagBits getSampleCount() {
    return multiSample ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT;
  }

  inline VkImage getVkImage() {
    return image_.image;
  }

  inline uint32_t getLevelCount() {
    return levelCount_;
  }

  inline uint32_t getLayerCount() {
    switch (type) {
      case TextureType_2D:
        return 1;
      case TextureType_CUBE:
        return 6;
    }
    return 0;
  }

  inline uint32_t getPixelByteSize() {
    switch (format) {
      case TextureFormat_RGBA8:
        return sizeof(RGBA);
      case TextureFormat_FLOAT32:
        return sizeof(float);
    }
    return 0;
  }

  inline uint32_t getImageAspect() {
    if (usage & TextureUsage_AttachmentDepth) {
      return VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    return VK_IMAGE_ASPECT_COLOR_BIT;
  }

  inline VkImageView &getSampleImageView() {
    if (sampleView_ == VK_NULL_HANDLE) {
      createImageView(sampleView_, image_.image);
    }
    return sampleView_;
  }

  VkImageView createResolveView();

  VkImageView createAttachmentView(VkImageAspectFlags aspect, uint32_t layer, uint32_t level);

  static void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
                                    VkImageSubresourceRange subresourceRange,
                                    VkAccessFlags srcMask,
                                    VkAccessFlags dstMask,
                                    VkImageLayout oldLayout,
                                    VkImageLayout newLayout,
                                    VkPipelineStageFlags srcStage,
                                    VkPipelineStageFlags dstStage);

 protected:
  void createImage();
  void createImageResolve();
  bool createImageHost(uint32_t level);
  void createImageView(VkImageView &view, VkImage &image);
  void generateMipmaps();
  void setImageDataInternal(const std::vector<const void *> &buffers, VkDeviceSize imageSize);

 protected:
  UUID<TextureVulkan> uuid_;
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  SamplerDesc samplerDesc_;
  bool needResolve_ = false;
  bool needMipmaps_ = false;

  uint32_t layerCount_ = 1;
  uint32_t levelCount_ = 1;
  uint32_t imageAspect_ = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
  VkFormat vkFormat_ = VK_FORMAT_MAX_ENUM;

  AllocatedImage image_{};
  AllocatedImage imageResolve_{};  // msaa resolve (only color)

  VkSampler sampler_ = VK_NULL_HANDLE;
  VkImageView sampleView_ = VK_NULL_HANDLE;

  // for image data upload
  AllocatedBuffer uploadStagingBuffer_{};

  // for memory dump
  AllocatedImage hostImage_{};
  uint32_t hostImageLevel_ = 0;
};

}
