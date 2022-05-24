/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Camera.h"
#include "renderer/Texture.h"
#include "renderer/Renderer.h"

namespace SoftGL {
namespace App {

class Environment {
 public:
  static void ConvertEquirectangular(Texture &tex_in, Texture *tex_out, int width = 0, int height = 0);

  static void GenerateIrradianceMap(Texture *tex_in, Texture *tex_out);

  static void GeneratePrefilterMap(Texture *tex_in, Texture *tex_out);

 private:
  static void GeneratePrefilterMapLod(Texture *tex_in, Texture *tex_out, int width, int height, int mip);

  static std::shared_ptr<Renderer> CreateCubeRender(Camera &camera, int width, int height);

  static void CubeRenderDraw(Renderer &renderer,
                             Camera &camera,
                             const std::function<void(int face, Buffer<glm::u8vec4> &buff)> &face_cb);
};

}
}