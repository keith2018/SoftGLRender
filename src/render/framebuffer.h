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

  // TODO MTR
  virtual void SetColorAttachment(std::shared_ptr<Texture> &color, int level) {
    color_tex_type = TextureType_2D;
    color_attachment_2d.tex = color;
    color_attachment_2d.level = level;
    color_ready = true;
  };

  virtual void SetColorAttachment(std::shared_ptr<Texture> &color, CubeMapFace face, int level) {
    color_tex_type = TextureType_CUBE;
    color_attachment_cube.tex = color;
    color_attachment_cube.face = face;
    color_attachment_cube.level = level;
    color_ready = true;
  };

  virtual void SetDepthAttachment(std::shared_ptr<Texture> &depth) {
    depth_attachment = depth;
    depth_ready = true;
  };

 protected:
  bool color_ready = false;
  bool depth_ready = false;

  TextureType color_tex_type = TextureType_2D;

  struct {
    std::shared_ptr<Texture> tex = nullptr;
    int level = 0;
  } color_attachment_2d;

  struct {
    std::shared_ptr<Texture> tex = nullptr;
    CubeMapFace face = TEXTURE_CUBE_MAP_POSITIVE_X;
    int level = 0;
  } color_attachment_cube;

  std::shared_ptr<Texture> depth_attachment = nullptr;
};

}
