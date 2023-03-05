/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "camera.h"
#include "model.h"
#include "render/texture.h"
#include "render/renderer.h"
#include "render/framebuffer.h"

namespace SoftGL {
namespace View {

constexpr int kIrradianceMapSize = 32;
constexpr int kPrefilterMaxMipLevels = 5;
constexpr int kPrefilterMapSize = 128;

struct CubeRenderContext {
  std::shared_ptr<Renderer> renderer;
  std::shared_ptr<FrameBuffer> fbo;
  Camera camera;
  ModelSkybox model_skybox;
  std::shared_ptr<UniformBlock> uniforms_block_mvp;
};

class Environment {
 public:
  static bool ConvertEquirectangular(const std::shared_ptr<Renderer> &renderer,
                                     const std::function<bool(ShaderProgram &program)> &shader_func,
                                     const std::shared_ptr<Texture> &tex_in,
                                     std::shared_ptr<Texture> &tex_out);

  static bool GenerateIrradianceMap(const std::shared_ptr<Renderer> &renderer,
                                    const std::function<bool(ShaderProgram &program)> &shader_func,
                                    const std::shared_ptr<Texture> &tex_in,
                                    std::shared_ptr<Texture> &tex_out);

  static bool GeneratePrefilterMap(const std::shared_ptr<Renderer> &renderer,
                                   const std::function<bool(ShaderProgram &program)> &shader_func,
                                   const std::shared_ptr<Texture> &tex_in,
                                   std::shared_ptr<Texture> &tex_out);

 private:
  static bool CreateCubeRenderContext(CubeRenderContext &context,
                                      const std::function<bool(ShaderProgram &program)> &shader_func,
                                      const std::shared_ptr<Texture> &tex_in,
                                      TextureUsage tex_usage);

  static void DrawCubeFaces(CubeRenderContext &context,
                            int width,
                            int height,
                            std::shared_ptr<Texture> &tex_out,
                            int tex_out_level = 0,
                            const std::function<void()> &before_draw = nullptr);
};

}
}
