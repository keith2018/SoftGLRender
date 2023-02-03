/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "renderer_soft.h"
#include "framebuffer_soft.h"
#include "texture_soft.h"
#include "uniform_soft.h"
#include "shader_program_soft.h"
#include "vertex_soft.h"

namespace SoftGL {

// framebuffer
std::shared_ptr<FrameBuffer> RendererSoft::CreateFrameBuffer() {
  return std::make_shared<FrameBufferSoft>();
}

// texture
std::shared_ptr<Texture2D> RendererSoft::CreateTexture2D() {
  return std::make_shared<Texture2DSoft>();
}

std::shared_ptr<TextureCube> RendererSoft::CreateTextureCube() {
  return std::make_shared<TextureCubeSoft>();
}

std::shared_ptr<TextureDepth> RendererSoft::CreateTextureDepth() {
  return std::make_shared<TextureDepthSoft>();
}

// vertex
std::shared_ptr<VertexArrayObject> RendererSoft::CreateVertexArrayObject(const VertexArray &vertex_array) {
  return std::make_shared<VertexArrayObjectSoft>(vertex_array);
}

// shader program
std::shared_ptr<ShaderProgram> RendererSoft::CreateShaderProgram() {
  return std::make_shared<ShaderProgramSoft>();
}

// uniform
std::shared_ptr<UniformBlock> RendererSoft::CreateUniformBlock(const std::string &name, int size) {
  return std::make_shared<UniformBlockSoft>(name, size);
}

std::shared_ptr<UniformSampler> RendererSoft::CreateUniformSampler(const std::string &name) {
  return std::make_shared<UniformSamplerSoft>(name);
}

// pipeline
void RendererSoft::SetFrameBuffer(FrameBuffer &frame_buffer) {
  fbo_ = dynamic_cast<FrameBufferSoft *>(&frame_buffer);
}

void RendererSoft::SetViewPort(int x, int y, int width, int height) {
  viewport_.x = (float) width / 2.f;
  viewport_.y = (float) height / 2.f;
  viewport_.z = (float) x + viewport_.x;
  viewport_.w = (float) y + viewport_.y;
}

void RendererSoft::Clear(const ClearState &state) {
  if (state.color_flag) {
    auto color_buffer = fbo_->GetColorBuffer();
    if (color_buffer) {
      color_buffer->SetAll(glm::u8vec4(state.clear_color.r * 255,
                                       state.clear_color.g * 255,
                                       state.clear_color.b * 255,
                                       state.clear_color.a * 255));
    }
  }

  if (state.depth_flag) {
    auto depth_buffer = fbo_->GetDepthBuffer();
    if (depth_buffer) {
      depth_buffer->SetAll(depth_range_.near);
    }
  }
}

void RendererSoft::SetRenderState(const RenderState &state) {
  render_state_ = &state;
}

void RendererSoft::SetVertexArray(VertexArray &vertex) {
  vertexArray_ = &vertex;
}

void RendererSoft::SetShaderProgram(ShaderProgram &program) {
  shader_program_ = dynamic_cast<ShaderProgramSoft *>(&program);
}

void RendererSoft::SetShaderUniforms(ShaderUniforms &uniforms) {
}

void RendererSoft::Draw(PrimitiveType type) {
  // vertex shader

  // primitive assembly

  // clipping

  // perspective divide

  // viewport transform

  // rasterization

  // fragment shader

  // depth test
}

void RendererSoft::SetDepthRange(float near, float far) {
  depth_range_.near = near;
  depth_range_.far = far;
  depth_range_.diff = far - near;
  depth_range_.sum = far + near;
}

}
