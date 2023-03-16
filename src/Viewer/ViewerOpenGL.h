/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <glad/glad.h>
#include "Viewer.h"
#include "Render/OpenGL/RendererOpenGL.h"
#include "Render/OpenGL/ShaderProgramOpenGL.h"
#include "Render/OpenGL/OpenGLUtils.h"
#include "Shader/GLSL/ShaderGLSL.h"

namespace SoftGL {
namespace View {

#define CASE_CREATE_SHADER_GL(shading, source) case shading: \
  return programGL->compileAndLink(source##_VS, source##_FS)

class ViewerOpenGL : public Viewer {
 public:
  ViewerOpenGL(Config &config, Camera &camera) : Viewer(config, camera) {
    GL_CHECK(glGenFramebuffers(1, &fbo_in_));
    GL_CHECK(glGenFramebuffers(1, &fbo_out_));
  }

  void configRenderer() override {
    // disabled
    config_.reverseZ = false;
    config_.earlyZ = false;
  }

  void swapBuffer() override {
    int width = texColorMain_->width;
    int height = texColorMain_->height;

    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_in_));
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                    GL_COLOR_ATTACHMENT0,
                                    texColorMain_->multiSample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
                                    texColorMain_->getId(),
                                    0));

    GL_CHECK(glBindTexture(GL_TEXTURE_2D, outTexId_));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_out_));
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTexId_, 0));

    GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_in_));
    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_out_));

    GL_CHECK(glBlitFramebuffer(0, 0, width, height,
                               0, 0, width, height,
                               GL_COLOR_BUFFER_BIT,
                               GL_NEAREST));
  }

  void destroy() override {
    if (renderer_) {
      renderer_->destroy();
    }
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
      CASE_CREATE_SHADER_GL(Shading_BaseColor, BASIC);
      CASE_CREATE_SHADER_GL(Shading_BlinnPhong, BLINN_PHONG);
      CASE_CREATE_SHADER_GL(Shading_PBR, PBR_IBL);
      CASE_CREATE_SHADER_GL(Shading_Skybox, SKYBOX);
      CASE_CREATE_SHADER_GL(Shading_IBL_Irradiance, IBL_IRRADIANCE);
      CASE_CREATE_SHADER_GL(Shading_IBL_Prefilter, IBL_PREFILTER);
      CASE_CREATE_SHADER_GL(Shading_FXAA, FXAA);
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
