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

struct FrameBufferContainerVK {
  std::vector<VkImageView> attachments;
  VkFramebuffer framebuffer = VK_NULL_HANDLE;
};

class FrameBufferVulkan : public FrameBuffer {
 public:
  FrameBufferVulkan(VKContext &ctx, bool offscreen) : FrameBuffer(offscreen), vkCtx_(ctx) {
    device_ = ctx.device();
  }

  virtual ~FrameBufferVulkan() {
    for (auto &pass : renderPassCache_) {
      vkDestroyRenderPass(device_, pass, nullptr);
    }

    for (auto &fbo : fboCache_) {
      vkDestroyFramebuffer(device_, fbo.framebuffer, nullptr);

      for (auto &view : fbo.attachments) {
        vkDestroyImageView(device_, view, nullptr);
      }
    }
  }

  int getId() const override {
    return uuid_.get();
  }

  bool isValid() override {
    return colorReady_ || depthReady_;
  }

  void setColorAttachment(std::shared_ptr<Texture> &color, int level) override {
    if (color == colorAttachment_.tex && level == colorAttachment_.level) {
      return;
    }

    fboDirty_ = true;
    if (color != colorAttachment_.tex) {
      renderPassDirty_ = true;
    }

    FrameBuffer::setColorAttachment(color, level);
    width_ = color->getLevelWidth(level);
    height_ = color->getLevelHeight(level);
  }

  void setColorAttachment(std::shared_ptr<Texture> &color, CubeMapFace face, int level) override {
    if (color == colorAttachment_.tex && face == colorAttachment_.layer && level == colorAttachment_.level) {
      return;
    }

    fboDirty_ = true;
    if (color != colorAttachment_.tex) {
      renderPassDirty_ = true;
    }

    FrameBuffer::setColorAttachment(color, face, level);
    width_ = color->getLevelWidth(level);
    height_ = color->getLevelHeight(level);
  }

  void setDepthAttachment(std::shared_ptr<Texture> &depth) override {
    if (depth == depthAttachment_.tex) {
      return;
    }

    fboDirty_ = true;
    renderPassDirty_ = true;

    FrameBuffer::setDepthAttachment(depth);
    width_ = depth->width;
    height_ = depth->height;
  }

  bool create(const ClearStates &states) {
    if (!isValid()) {
      return false;
    }
    clearStates_ = states;

    bool success = true;

    if (renderPassDirty_) {
      renderPassDirty_ = false;
      renderPassCache_.emplace_back();
      currRenderPass_ = &renderPassCache_.back();
      success = createVkRenderPass();
    }
    if (success && fboDirty_) {
      fboDirty_ = false;
      fboCache_.emplace_back();
      currFbo_ = &fboCache_.back();
      success = createVkFramebuffer();
    }

    return success;
  }

  inline ClearStates &getClearStates() {
    return clearStates_;
  }

  inline VkRenderPass &getRenderPass() {
    return *currRenderPass_;
  }

  inline VkFramebuffer &getVkFramebuffer() {
    return currFbo_->framebuffer;
  }

  inline VkSampleCountFlagBits getSampleCount() {
    if (colorReady_) {
      return getAttachmentColor()->getSampleCount();
    }

    if (depthReady_) {
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

  inline TextureVulkan *getAttachmentColor() {
    if (colorReady_) {
      return dynamic_cast<TextureVulkan *>(colorAttachment_.tex.get());
    }
    return nullptr;
  }

  inline TextureVulkan *getAttachmentDepth() {
    if (depthReady_) {
      return dynamic_cast<TextureVulkan *>(depthAttachment_.tex.get());
    }
    return nullptr;
  }

  void transitionLayoutBeginPass(VkCommandBuffer cmdBuffer);
  void transitionLayoutEndPass(VkCommandBuffer cmdBuffer);

  std::vector<VkSemaphore> &getAttachmentsSemaphoresWait();
  std::vector<VkSemaphore> &getAttachmentsSemaphoresSignal();

 private:
  bool createVkRenderPass();
  bool createVkFramebuffer();

 private:
  UUID<FrameBufferVulkan> uuid_;
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  uint32_t width_ = 0;
  uint32_t height_ = 0;

  bool renderPassDirty_ = true;
  bool fboDirty_ = true;

  ClearStates clearStates_{};

  VkRenderPass *currRenderPass_ = nullptr;
  FrameBufferContainerVK *currFbo_ = nullptr;

  std::vector<VkSemaphore> attachmentsSemaphoresWait_;
  std::vector<VkSemaphore> attachmentsSemaphoresSignal_;

  // TODO purge outdated elements
  std::vector<VkRenderPass> renderPassCache_;
  std::vector<FrameBufferContainerVK> fboCache_;
};

}
