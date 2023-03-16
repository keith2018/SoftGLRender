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
  int getId() const override {
    return uuid_.get();
  }

  bool isValid() override {
    return colorReady || depthReady;
  }

  std::shared_ptr<ImageBufferSoft<RGBA>> getColorBuffer() const {
    if (!colorReady) {
      return nullptr;
    }

    switch (colorTexType) {
      case TextureType_2D: {
        auto *color2d = dynamic_cast<Texture2DSoft<RGBA> *>(colorAttachment2d.tex.get());
        return color2d->getImage().getBuffer(colorAttachment2d.level);
      }
      case TextureType_CUBE: {
        auto *colorCube = dynamic_cast<TextureCubeSoft<RGBA> *>(colorAttachmentCube.tex.get());
        return colorCube->getImage(colorAttachmentCube.face).getBuffer(colorAttachmentCube.level);
      }
      default:
        break;
    }

    return nullptr;
  };

  std::shared_ptr<ImageBufferSoft<float>> getDepthBuffer() const {
    if (!depthReady) {
      return nullptr;
    }
    auto *depthTex = dynamic_cast<Texture2DSoft<float> *>(depthAttachment.get());
    return depthTex->getImage().getBuffer();
  };

 private:
  UUID<FrameBufferSoft> uuid_;
};

}
