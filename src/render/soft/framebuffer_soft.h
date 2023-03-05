/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/framebuffer.h"
#include "render/soft/texture_soft.h"

namespace SoftGL {

class FrameBufferSoft : public FrameBuffer {
 public:
  FrameBufferSoft() : uuid_(uuid_counter_++) {
  }

  ~FrameBufferSoft() = default;

  int GetId() const override {
    return uuid_;
  }

  bool IsValid() override {
    return color_ready || depth_ready;
  }

  std::shared_ptr<ImageBufferSoft<RGBA>> GetColorBuffer() const {
    if (!color_ready) {
      return nullptr;
    }

    switch (color_tex_type) {
      case TextureType_2D: {
        auto *color_2d = dynamic_cast<Texture2DSoft<RGBA> *>(color_attachment_2d.tex.get());
        return color_2d->GetImage().GetBuffer(color_attachment_2d.level);
      }
      case TextureType_CUBE: {
        auto *color_cube = dynamic_cast<TextureCubeSoft<RGBA> *>(color_attachment_cube.tex.get());
        return color_cube->GetImage(color_attachment_cube.face).GetBuffer(color_attachment_cube.level);
      }
      default:
        break;
    }

    return nullptr;
  };

  std::shared_ptr<ImageBufferSoft<float>> GetDepthBuffer() const {
    if (!depth_ready) {
      return nullptr;
    }
    auto *depth_tex = dynamic_cast<Texture2DSoft<float> *>(depth_attachment.get());
    return depth_tex->GetImage().GetBuffer();
  };

 private:
  int uuid_ = -1;
  static int uuid_counter_;
};

}
