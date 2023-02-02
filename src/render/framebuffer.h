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

  virtual void SetColorAttachment(std::shared_ptr<Texture2D> &color, int level) {
    color_tex_type = TextureType_2D;
    color_attachment_2d.tex = color;
    color_attachment_2d.level = level;
    color_ready = true;
  };

  virtual void SetColorAttachment(std::shared_ptr<TextureCube> &color, CubeMapFace face, int level) {
    color_tex_type = TextureType_CUBE;
    color_attachment_cube.tex = color;
    color_attachment_cube.face = face;
    color_attachment_cube.level = level;
    color_ready = true;
  };

  virtual void SetDepthAttachment(std::shared_ptr<TextureDepth> &depth) {
    depth_attachment = depth;
    depth_ready = true;
  };

  void UpdateAttachmentsSize(int width, int height) {
    // color attachment
    if (color_ready) {
      UpdateColorAttachmentsSize(width, height);
    }

    // depth attachment
    if (depth_ready) {
      UpdateDepthAttachmentsSize(width, height);
    }
  }

  void UpdateColorAttachmentsSize(int width, int height) {
    switch (color_tex_type) {
      case TextureType_2D:
        if (color_attachment_2d.tex->width != width || color_attachment_2d.tex->height != height) {
          color_attachment_2d.tex->InitImageData(width, height);
          SetColorAttachment(color_attachment_2d.tex, color_attachment_2d.level);
        }
        break;
      case TextureType_CUBE:
        if (color_attachment_cube.tex->width != width || color_attachment_cube.tex->height != height) {
          color_attachment_cube.tex->InitImageData(width, height);
          SetColorAttachment(color_attachment_cube.tex, color_attachment_cube.face, color_attachment_cube.level);
        }
        break;
      default:
        break;
    }
  }

  void UpdateDepthAttachmentsSize(int width, int height) {
    if (depth_attachment->width != width || depth_attachment->height != height) {
      depth_attachment->InitImageData(width, height);
      SetDepthAttachment(depth_attachment);
    }
  }

 protected:
  bool color_ready = false;
  bool depth_ready = false;

  TextureType color_tex_type = TextureType_2D;

  struct {
    std::shared_ptr<Texture2D> tex = nullptr;
    int level = 0;
  } color_attachment_2d;

  struct {
    std::shared_ptr<TextureCube> tex = nullptr;
    CubeMapFace face = TEXTURE_CUBE_MAP_POSITIVE_X;
    int level = 0;
  } color_attachment_cube;

  std::shared_ptr<TextureDepth> depth_attachment = nullptr;
};

}
