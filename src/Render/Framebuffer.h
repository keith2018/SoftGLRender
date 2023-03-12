/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <memory>
#include "Texture.h"

namespace SoftGL {

class FrameBuffer {
 public:
  virtual int getId() const = 0;
  virtual bool isValid() = 0;

  // TODO MTR
  virtual void setColorAttachment(std::shared_ptr<Texture> &color, int level) {
    colorTexType = TextureType_2D;
    colorAttachment2d.tex = color;
    colorAttachment2d.level = level;
    colorReady = true;
  };

  virtual void setColorAttachment(std::shared_ptr<Texture> &color, CubeMapFace face, int level) {
    colorTexType = TextureType_CUBE;
    colorAttachmentCube.tex = color;
    colorAttachmentCube.face = face;
    colorAttachmentCube.level = level;
    colorReady = true;
  };

  virtual void setDepthAttachment(std::shared_ptr<Texture> &depth) {
    depthAttachment = depth;
    depthReady = true;
  };

 protected:
  bool colorReady = false;
  bool depthReady = false;

  TextureType colorTexType = TextureType_2D;

  struct {
    std::shared_ptr<Texture> tex = nullptr;
    int level = 0;
  } colorAttachment2d;

  struct {
    std::shared_ptr<Texture> tex = nullptr;
    CubeMapFace face = TEXTURE_CUBE_MAP_POSITIVE_X;
    int level = 0;
  } colorAttachmentCube;

  std::shared_ptr<Texture> depthAttachment = nullptr;
};

}
