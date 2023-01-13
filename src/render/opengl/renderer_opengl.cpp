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


namespace SoftGL {

// framebuffer
std::shared_ptr<FrameBuffer> RendererOpenGL::CreateFrameBuffer() {
  return std::make_shared<FrameBufferOpenGL>();
}

// texture
std::shared_ptr<Texture2D> RendererOpenGL::CreateTexture2DRef(int refId) {
  return std::make_shared<Texture2DOpenGL>(refId);
}

std::shared_ptr<Texture2D> RendererOpenGL::CreateTexture2D() {
  return std::make_shared<Texture2DOpenGL>();
}

std::shared_ptr<TextureDepth> RendererOpenGL::CreateDepthTexture() {
  return std::make_shared<TextureDepthOpenGL>();
}

// vertex
std::shared_ptr<VertexArrayObject> RendererOpenGL::CreateVertexArrayObject(VertexArray &vertex_array) {
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

std::shared_ptr<UniformSampler> RendererOpenGL::CreateUniformSampler(const std::string &name) {
  return std::make_shared<UniformSamplerOpenGL>(name);
}

// material

// pipeline
void RendererOpenGL::SetViewPort(int x, int y, int width, int height) {
  glViewport(x, y, width, height);
}

void RendererOpenGL::Clear(ClearState &state) {
  glClearColor(state.clear_color.r,
               state.clear_color.g,
               state.clear_color.b,
               state.clear_color.a);
  GLbitfield clear_bit = 0;
  if (state.color_flag) {
    clear_bit = clear_bit | GL_COLOR_BUFFER_BIT;
  }
  if (state.depth_flag) {
    clear_bit = clear_bit | GL_DEPTH_BUFFER_BIT;
  }
  glClear(clear_bit);
}

void RendererOpenGL::SetRenderState(RenderState &state) {
  state.blend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
  glBlendFunc(OpenGL::ConvertBlendFactor(state.blend_src), OpenGL::ConvertBlendFactor(state.blend_dst));

  state.depth_test ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
  glDepthMask(state.depth_mask);
  glDepthFunc(OpenGL::ConvertDepthFunc(state.depth_func));

  state.cull_face ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
  glPolygonMode(GL_FRONT_AND_BACK, OpenGL::ConvertPolygonMode(state.polygon_mode));

  glLineWidth(state.line_width);
  glPointSize(state.point_size);
}

void RendererOpenGL::SetVertexArray(VertexArray &vertex) {
  vertexArray_ = &vertex;
  vertex.vao->Bind();
}

void RendererOpenGL::Draw(PrimitiveType type) {
  GLenum mode = OpenGL::ConvertPrimitiveType(type);
  glDrawElements(mode, (GLsizei) vertexArray_->indices.size(), GL_UNSIGNED_INT, nullptr);
}

}
