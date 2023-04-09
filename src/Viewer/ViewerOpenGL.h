/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <glad/glad.h>
#include "Viewer.h"
#include "Config.h"
#include "Render/OpenGL/RendererOpenGL.h"
#include "Render/OpenGL/ShaderProgramOpenGL.h"
#include "Render/OpenGL/OpenGLUtils.h"

namespace SoftGL {
namespace View {

#define CASE_CREATE_SHADER_GL(shading, source) case shading: \
  return programGL->compileAndLinkFile(SHADER_GLSL_DIR + #source + ".vert", \
                                       SHADER_GLSL_DIR + #source + ".frag")

class ViewerOpenGL : public Viewer {
 public:
  ViewerOpenGL(Config &config, Camera &camera) : Viewer(config, camera) {
    GL_CHECK(glGenFramebuffers(1, &fbo_in_));
    GL_CHECK(glGenFramebuffers(1, &fbo_out_));
  }

  void configRenderer() override {
    // disabled
    config_.reverseZ = false;

    camera_->setReverseZ(config_.reverseZ);
    cameraDepth_->setReverseZ(config_.reverseZ);
  }

  int swapBuffer() override {
    int width = texColorMain_->width;
    int height = texColorMain_->height;

    if (texColorMain_->multiSample) {
      GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_in_));
      GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                      GL_COLOR_ATTACHMENT0,
                                      GL_TEXTURE_2D_MULTISAMPLE,
                                      texColorMain_->getId(),
                                      0));

      GL_CHECK(glBindTexture(GL_TEXTURE_2D, outTexId_));
      GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_out_));
      GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTexId_, 0));

      GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_in_));
      GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_out_));

      GL_CHECK(glBlitFramebuffer(0, 0, width, height,
                                 0, 0, width, height,
                                 GL_COLOR_BUFFER_BIT,
                                 GL_NEAREST));
      return outTexId_;
    }

    return texColorMain_->getId();
  }

  void destroy() override {
    Viewer::destroy();

    GL_CHECK(glDeleteFramebuffers(1, &fbo_in_));
    GL_CHECK(glDeleteFramebuffers(1, &fbo_out_));
  }

  std::shared_ptr<Renderer> createRenderer() override {
    auto renderer = std::make_shared<RendererOpenGL>();
    if (!renderer->create()) {
      return nullptr;
    }
    return renderer;
  }

  bool loadShaders(ShaderProgram &program, ShadingModel shading) override {
    auto *programGL = dynamic_cast<ShaderProgramOpenGL *>(&program);
    switch (shading) {
      CASE_CREATE_SHADER_GL(Shading_BaseColor, BasicGLSL);
      CASE_CREATE_SHADER_GL(Shading_BlinnPhong, BlinnPhongGLSL);
      CASE_CREATE_SHADER_GL(Shading_PBR, PbrGLSL);
      CASE_CREATE_SHADER_GL(Shading_Skybox, SkyboxGLSL);
      CASE_CREATE_SHADER_GL(Shading_IBL_Irradiance, IBLIrradianceGLSL);
      CASE_CREATE_SHADER_GL(Shading_IBL_Prefilter, IBLPrefilterGLSL);
      CASE_CREATE_SHADER_GL(Shading_FXAA, FxaaGLSL);
      default:
        break;
    }

    return false;
  }

 private:
  GLuint fbo_in_ = 0;
  GLuint fbo_out_ = 0;
};

}
}
