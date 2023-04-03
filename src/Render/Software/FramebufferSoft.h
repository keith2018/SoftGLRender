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
    return colorReady_ || depthReady_;
  }

  std::shared_ptr<ImageBufferSoft<RGBA>> getColorBuffer() const {
    if (!colorReady_) {
      return nullptr;
    }

    switch (colorAttachment_.tex->type) {
      case TextureType_2D: {
        auto *color2d = dynamic_cast<Texture2DSoft<RGBA> *>(colorAttachment_.tex.get());
        return color2d->getImage().getBuffer(colorAttachment_.level);
      }
      case TextureType_CUBE: {
        auto *colorCube = dynamic_cast<TextureCubeSoft<RGBA> *>(colorAttachment_.tex.get());
        return colorCube->getImage(static_cast<CubeMapFace>(colorAttachment_.layer)).getBuffer(colorAttachment_.level);
      }
      default:
        break;
    }

    return nullptr;
  };

  std::shared_ptr<ImageBufferSoft<float>> getDepthBuffer() const {
    if (!depthReady_) {
      return nullptr;
    }
    auto *depthTex = dynamic_cast<Texture2DSoft<float> *>(depthAttachment_.tex.get());
    return depthTex->getImage().getBuffer();
  };

 private:
  UUID<FrameBufferSoft> uuid_;
};

}
