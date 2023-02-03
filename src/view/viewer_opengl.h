/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <glad/glad.h>
#include "viewer.h"
#include "render/opengl/renderer_opengl.h"
#include "render/opengl/shader_program_opengl.h"
#include "render/opengl/opengl_utils.h"
#include "shader/opengl/shader_glsl.h"

namespace SoftGL {
namespace View {

class ViewerOpenGL : public Viewer {
 public:
  ViewerOpenGL(Config &config, Camera &camera) : Viewer(config, camera) {
    GL_CHECK(glGenFramebuffers(1, &fbo_));
  }

  void SwapBuffer() override {
    int width = color_tex_out_->width;
    int height = color_tex_out_->height;

    GL_CHECK(glBindTexture(GL_TEXTURE_2D, outTexId_));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));

    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex_out_->GetId(), 0));
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, outTexId_, 0));

    GL_CHECK(glReadBuffer(GL_COLOR_ATTACHMENT0));
    GL_CHECK(glDrawBuffer(GL_COLOR_ATTACHMENT1));

    GL_CHECK(glBlitFramebuffer(0, 0, width, height,
                               0, 0, width, height,
                               GL_COLOR_BUFFER_BIT,
                               GL_LINEAR));
  }

  void Destroy() override {
    GL_CHECK(glDeleteFramebuffers(1, &fbo_));
  }

  std::shared_ptr<Renderer> CreateRenderer() override {
    return std::make_shared<RendererOpenGL>();
  }

  bool LoadShaders(ShaderProgram &program, ShadingModel shading) override {
    auto *program_gl = dynamic_cast<ShaderProgramOpenGL *>(&program);
    switch (shading) {
      case Shading_BaseColor:
        return program_gl->CompileAndLink(BASIC_VS, BASIC_FS);
      case Shading_BlinnPhong:
        return program_gl->CompileAndLink(BLINN_PHONG_VS, BLINN_PHONG_FS);
      case Shading_PBR:
        return program_gl->CompileAndLink(PBR_IBL_VS, PBR_IBL_FS);
      case Shading_Skybox:
        return program_gl->CompileAndLink(SKYBOX_VS, SKYBOX_FS);
      case Shading_IBL_Irradiance:
        return program_gl->CompileAndLink(IBL_IRRADIANCE_VS, IBL_IRRADIANCE_FS);
      case Shading_IBL_Prefilter:
        return program_gl->CompileAndLink(IBL_PREFILTER_VS, IBL_PREFILTER_FS);
      case Shading_FXAA:
        return program_gl->CompileAndLink(FXAA_VS, FXAA_FS);
      default:
        break;
    }

    return false;
  }

 private:
  GLuint fbo_ = 0;
};

}
}
