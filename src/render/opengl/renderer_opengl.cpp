/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "renderer_opengl.h"
#include "framebuffer_opengl.h"
#include "texture_opengl.h"
#include "uniform_opengl.h"
#include "shader_program_opengl.h"
#include "vertex_opengl.h"
#include "enums_opengl.h"

#define GL_STATE_SET(var, gl_state) if (var) GL_CHECK(glEnable(gl_state)); else GL_CHECK(glDisable(gl_state));

namespace SoftGL {

// framebuffer
std::shared_ptr<FrameBuffer> RendererOpenGL::CreateFrameBuffer() {
  return std::make_shared<FrameBufferOpenGL>();
}

// texture
std::shared_ptr<Texture2D> RendererOpenGL::CreateTexture2D(bool multi_sample) {
  return std::make_shared<Texture2DOpenGL>(multi_sample);
}

std::shared_ptr<TextureCube> RendererOpenGL::CreateTextureCube() {
  return std::make_shared<TextureCubeOpenGL>();
}

std::shared_ptr<TextureDepth> RendererOpenGL::CreateTextureDepth(bool multi_sample) {
  return std::make_shared<TextureDepthOpenGL>(multi_sample);
}

// vertex
std::shared_ptr<VertexArrayObject> RendererOpenGL::CreateVertexArrayObject(const VertexArray &vertex_array) {
  return std::make_shared<VertexArrayObjectOpenGL>(vertex_array);
}

// shader program
std::shared_ptr<ShaderProgram> RendererOpenGL::CreateShaderProgram() {
  return std::make_shared<ShaderProgramOpenGL>();
}

// uniform
std::shared_ptr<UniformBlock> RendererOpenGL::CreateUniformBlock(const std::string &name, int size) {
  return std::make_shared<UniformBlockOpenGL>(name, size);
}

std::shared_ptr<UniformSampler> RendererOpenGL::CreateUniformSampler(const std::string &name, TextureType type) {
  return std::make_shared<UniformSamplerOpenGL>(name, type);
}

// pipeline
void RendererOpenGL::SetFrameBuffer(FrameBuffer &frame_buffer) {
  auto &fbo = dynamic_cast<FrameBufferOpenGL &>(frame_buffer);
  fbo.Bind();
}

void RendererOpenGL::SetViewPort(int x, int y, int width, int height) {
  GL_CHECK(glViewport(x, y, width, height));
}

void RendererOpenGL::Clear(const ClearState &state) {
  GL_CHECK(glClearColor(state.clear_color.r,
                        state.clear_color.g,
                        state.clear_color.b,
                        state.clear_color.a));
  GLbitfield clear_bit = 0;
  if (state.color_flag) {
    clear_bit = clear_bit | GL_COLOR_BUFFER_BIT;
  }
  if (state.depth_flag) {
    clear_bit = clear_bit | GL_DEPTH_BUFFER_BIT;
  }
  GL_CHECK(glClear(clear_bit));
}

void RendererOpenGL::SetRenderState(const RenderState &state) {
  // blend
  GL_STATE_SET(state.blend, GL_BLEND)
  GL_CHECK(glBlendEquationSeparate(OpenGL::ConvertBlendFunction(state.blend_parameters.blend_func_rgb),
                                   OpenGL::ConvertBlendFunction(state.blend_parameters.blend_func_alpha)));
  GL_CHECK(glBlendFuncSeparate(OpenGL::ConvertBlendFactor(state.blend_parameters.blend_src_rgb),
                               OpenGL::ConvertBlendFactor(state.blend_parameters.blend_dst_rgb),
                               OpenGL::ConvertBlendFactor(state.blend_parameters.blend_src_alpha),
                               OpenGL::ConvertBlendFactor(state.blend_parameters.blend_dst_alpha)));

  // depth
  GL_STATE_SET(state.depth_test, GL_DEPTH_TEST)
  GL_CHECK(glDepthMask(state.depth_mask));
  GL_CHECK(glDepthFunc(OpenGL::ConvertDepthFunc(state.depth_func)));

  GL_STATE_SET(state.cull_face, GL_CULL_FACE)
  GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, OpenGL::ConvertPolygonMode(state.polygon_mode)));

  GL_CHECK(glLineWidth(state.line_width));
  GL_CHECK(glPointSize(state.point_size));
}

void RendererOpenGL::SetVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) {
  if (!vao) {
    return;
  }
  vao_ = dynamic_cast<VertexArrayObjectOpenGL *>(vao.get());
  vao_->Bind();
}

void RendererOpenGL::SetShaderProgram(std::shared_ptr<ShaderProgram> &program) {
  if (!program) {
    return;
  }
  shader_program_ = dynamic_cast<ShaderProgramOpenGL *>(program.get());
  shader_program_->Use();
}

void RendererOpenGL::SetShaderUniforms(std::shared_ptr<ShaderUniforms> &uniforms) {
  if (!uniforms) {
    return;
  }
  if (shader_program_) {
    shader_program_->BindUniforms(*uniforms);
  }
}

void RendererOpenGL::Draw(PrimitiveType type) {
  GLenum mode = OpenGL::ConvertDrawMode(type);
  GL_CHECK(glDrawElements(mode, (GLsizei) vao_->GetIndicesCnt(), GL_UNSIGNED_INT, nullptr));
}

}
