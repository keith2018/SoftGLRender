/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "camera.h"
#include "render/soft/sampler.h"
#include "render/soft/renderer_soft.h"

namespace SoftGL {
namespace View {

class Environment {
 public:
  static void ConvertEquirectangular(Texture &tex_in, Texture *tex_out, int width = 0, int height = 0);

  static void GenerateIrradianceMap(Texture *tex_in, Texture *tex_out);

  static void GeneratePrefilterMap(Texture *tex_in, Texture *tex_out);

 private:
  static void GeneratePrefilterMapLod(Texture *tex_in, Texture *tex_out, int width, int height, int mip);

  static std::shared_ptr<RendererSoft> CreateCubeRender(Camera &camera, int width, int height);

  static void CubeRenderDraw(RendererSoft &renderer,
                             Camera &camera,
                             const std::function<void(int face, Buffer<glm::u8vec4> &buff)> &face_cb);
};

}
}