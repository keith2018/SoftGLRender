/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <memory>
#include "texture.h"

namespace SoftGL {

class FrameBuffer {
 public:
  virtual bool IsValid() = 0;
  virtual void Bind() = 0;
  virtual void Unbind() = 0;

  virtual void SetColorAttachment(std::shared_ptr<Texture2D> &color) = 0;
  virtual void SetDepthAttachment(std::shared_ptr<TextureDepth> &depth) = 0;
};

}
