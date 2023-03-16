/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/UUID.h"
#include "Render/Framebuffer.h"
#include "Render/Vulkan/TextureVulkan.h"
#include "VulkanUtils.h"

namespace SoftGL {

class FrameBufferVulkan : public FrameBuffer {
 public:
  int getId() const override {
    return uuid_.get();
  }

  bool isValid() override {

    return true;
  }

  void setColorAttachment(std::shared_ptr<Texture> &color, int level) override {

    FrameBuffer::setColorAttachment(color, level);
  }

  void setColorAttachment(std::shared_ptr<Texture> &color, CubeMapFace face, int level) override {

    FrameBuffer::setColorAttachment(color, face, level);
  }

  void setDepthAttachment(std::shared_ptr<Texture> &depth) override {

    FrameBuffer::setDepthAttachment(depth);
  }

 private:
  UUID<FrameBufferVulkan> uuid_;
};

}
