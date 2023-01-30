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
  virtual int GetId() const = 0;
  virtual bool IsValid() = 0;

  virtual void SetColorAttachment(std::shared_ptr<Texture2D> &color, int level) = 0;
  virtual void SetColorAttachment(std::shared_ptr<TextureCube> &color, CubeMapFace face, int level) = 0;
  virtual void SetDepthAttachment(std::shared_ptr<TextureDepth> &depth) = 0;

  virtual std::shared_ptr<Texture> GetColorAttachment() = 0;
  virtual std::shared_ptr<Texture> GetDepthAttachment() = 0;

  virtual void UpdateAttachmentsSize(int width, int height) = 0;
};

}
