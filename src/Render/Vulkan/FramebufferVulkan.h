/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/UUID.h"
#include "Render/Framebuffer.h"
#include "Render/Vulkan/TextureVulkan.h"
#include "VKContext.h"

namespace SoftGL {

class FrameBufferVulkan : public FrameBuffer {
 public:
  explicit FrameBufferVulkan(VKContext &ctx) : vkCtx_(ctx) {
    device_ = ctx.device();
  }

  virtual ~FrameBufferVulkan() {
    vkDestroyFramebuffer(device_, framebuffer_, nullptr);
    vkDestroyRenderPass(device_, renderPass_, nullptr);
  }

  int getId() const override {
    return uuid_.get();
  }

  bool isValid() override {
    return colorReady || depthReady;
  }

  void setColorAttachment(std::shared_ptr<Texture> &color, int level) override {
    FrameBuffer::setColorAttachment(color, level);
    width_ = color->width;
    height_ = color->height;
    dirty_ = true;
  }

  void setColorAttachment(std::shared_ptr<Texture> &color, CubeMapFace face, int level) override {
    FrameBuffer::setColorAttachment(color, face, level);
    width_ = color->width;
    height_ = color->height;
    dirty_ = true;
  }

  void setDepthAttachment(std::shared_ptr<Texture> &depth) override {
    FrameBuffer::setDepthAttachment(depth);
    width_ = depth->width;
    height_ = depth->height;
    dirty_ = true;
  }

  bool create(const ClearStates &states) {
    if (!isValid()) {
      return false;
    }

    clearStates_ = states;
    if (dirty_) {
      dirty_ = false;
      createVkRenderPass();
      return createVkFramebuffer();
    }

    return true;
  }

  inline ClearStates &getClearStates() {
    return clearStates_;
  }

  inline VkRenderPass &getRenderPass() {
    return renderPass_;
  }

  inline VkFramebuffer &getVkFramebuffer() {
    return framebuffer_;
  }

  inline VkSampleCountFlagBits getSampleCount() {
    if (colorReady) {
      return getAttachmentColor()->getSampleCount();
    }

    if (depthReady) {
      return getAttachmentDepth()->getSampleCount();
    }

    return VK_SAMPLE_COUNT_1_BIT;
  }

  inline uint32_t width() const {
    return width_;
  }

  inline uint32_t height() const {
    return height_;
  }

 private:
  void createVkRenderPass();
  bool createVkFramebuffer();

  inline Texture2DVulkan *getAttachmentColor() {
    switch (colorTexType) {
      case TextureType_2D: {
        return dynamic_cast<Texture2DVulkan *>(colorAttachment2d.tex.get());
      }
      case TextureType_CUBE: {
        return dynamic_cast<Texture2DVulkan *>(colorAttachmentCube.tex.get());
      }
      default:
        break;
    }

    return nullptr;
  }

  inline Texture2DVulkan *getAttachmentDepth() {
    return dynamic_cast<Texture2DVulkan *>(depthAttachment.get());
  }

 private:
  UUID<FrameBufferVulkan> uuid_;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  bool dirty_ = true;

  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
  VkRenderPass renderPass_ = VK_NULL_HANDLE;

  ClearStates clearStates_{};
};

}
