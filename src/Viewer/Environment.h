/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Camera.h"
#include "Model.h"
#include "Render/Texture.h"
#include "Render/Renderer.h"
#include "Render/Framebuffer.h"

namespace SoftGL {
namespace View {

constexpr int kIrradianceMapSize = 32;
constexpr int kPrefilterMaxMipLevels = 5;
constexpr int kPrefilterMapSize = 128;

struct CubeRenderContext {
  std::shared_ptr<Renderer> renderer;
  std::shared_ptr<FrameBuffer> fbo;
  Camera camera;
  ModelSkybox modelSkybox;
  std::shared_ptr<UniformBlock> uniformsBlockModel;
};

class Environment {
 public:
  static bool convertEquirectangular(const std::shared_ptr<Renderer> &renderer,
                                     const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                     const std::shared_ptr<Texture> &texIn,
                                     std::shared_ptr<Texture> &texOut);

  static bool generateIrradianceMap(const std::shared_ptr<Renderer> &renderer,
                                    const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                    const std::shared_ptr<Texture> &texIn,
                                    std::shared_ptr<Texture> &texOut);

  static bool generatePrefilterMap(const std::shared_ptr<Renderer> &renderer,
                                   const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                   const std::shared_ptr<Texture> &texIn,
                                   std::shared_ptr<Texture> &texOut);

 private:
  static bool createCubeRenderContext(CubeRenderContext &context,
                                      const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                      const std::shared_ptr<Texture> &texIn,
                                      TextureUsage texUsage);

  static void drawCubeFaces(CubeRenderContext &context, int width, int height, std::shared_ptr<Texture> &texOut,
                            int texOutLevel = 0, const std::function<void()> &beforeDraw = nullptr);
};

}
}
