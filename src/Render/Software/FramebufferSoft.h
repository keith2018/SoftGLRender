/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/UUID.h"
#include "Render/Framebuffer.h"
#include "Render/Software/TextureSoft.h"

namespace SoftGL {

class FrameBufferSoft : public FrameBuffer {
 public:
  explicit FrameBufferSoft(bool offscreen) : FrameBuffer(offscreen) {}

  int getId() const override {
    return uuid_.get();
  }

  bool isValid() override {
    return colorReady_ || depthReady_;
  }

  std::shared_ptr<ImageBufferSoft<RGBA>> getColorBuffer() const {
    if (!colorReady_) {
      return nullptr;
    }
    auto *colorTex = dynamic_cast<TextureSoft<RGBA> *>(colorAttachment_.tex.get());
    return colorTex->getImage(colorAttachment_.layer).getBuffer(colorAttachment_.level);
  };

  std::shared_ptr<ImageBufferSoft<float>> getDepthBuffer() const {
    if (!depthReady_) {
      return nullptr;
    }
    auto *depthTex = dynamic_cast<TextureSoft<float> *>(depthAttachment_.tex.get());
    return depthTex->getImage(depthAttachment_.layer).getBuffer(depthAttachment_.level);
  };

 private:
  UUID<FrameBufferSoft> uuid_;
};

}
