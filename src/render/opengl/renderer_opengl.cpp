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

std::shared_ptr<TextureCube> RendererOpenGL::CreateTextureCube() {
  return std::make_shared<TextureCubeOpenGL>();
}

std::shared_ptr<TextureDepth> RendererOpenGL::CreateTextureDepth() {
  return std::make_shared<TextureDepthOpenGL>();
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

std::shared_ptr<UniformSampler> RendererOpenGL::CreateUniformSampler(const std::string &name) {
  return std::make_shared<UniformSamplerOpenGL>(name);
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
#define GL_STATE_SET(var, gl_state) if (var) GL_CHECK(glEnable(gl_state)); else GL_CHECK(glDisable(gl_state));

  GL_STATE_SET(state.blend, GL_BLEND)
  GL_CHECK(glBlendFunc(OpenGL::ConvertBlendFactor(state.blend_src), OpenGL::ConvertBlendFactor(state.blend_dst)));

  GL_STATE_SET(state.depth_test, GL_DEPTH_TEST)
  GL_CHECK(glDepthMask(state.depth_mask));
  GL_CHECK(glDepthFunc(OpenGL::ConvertDepthFunc(state.depth_func)));

  GL_STATE_SET(state.cull_face, GL_CULL_FACE)
  GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, OpenGL::ConvertPolygonMode(state.polygon_mode)));

  GL_CHECK(glLineWidth(state.line_width));
  GL_CHECK(glPointSize(state.point_size));
}

void RendererOpenGL::SetVertexArray(VertexArray &vertex) {
  vertexArray_ = &vertex;
  auto vao = std::dynamic_pointer_cast<VertexArrayObjectOpenGL>(vertex.vao);
  vao->Bind();
}

void RendererOpenGL::SetShaderProgram(ShaderProgram &program, ProgramUniforms &uniforms) {
  auto &program_gl = dynamic_cast<ShaderProgramOpenGL &>(program);
  program_gl.Use();
  program_gl.BindUniforms(uniforms);
}

void RendererOpenGL::Draw(PrimitiveType type) {
  GLenum mode = OpenGL::ConvertPrimitiveType(type);
  GL_CHECK(glDrawElements(mode, (GLsizei) vertexArray_->indices.size(), GL_UNSIGNED_INT, nullptr));
}

}
