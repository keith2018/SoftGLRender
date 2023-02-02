/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/renderer.h"
#include "render/soft/framebuffer_soft.h"
#include "render/soft/shader_program_soft.h"

namespace SoftGL {

struct DepthRange {
  float near = 0.f;
  float far = 1.f;
  float diff = 1.f;   // far - near
  float sum = 1.f;    // far + near
};

class RendererSoft : public Renderer {
 public:
  // framebuffer
  std::shared_ptr<FrameBuffer> CreateFrameBuffer() override;

  // texture
  std::shared_ptr<Texture2D> CreateTexture2D() override;
  std::shared_ptr<TextureCube> CreateTextureCube() override;
  std::shared_ptr<TextureDepth> CreateTextureDepth() override;

  // vertex
  std::shared_ptr<VertexArrayObject> CreateVertexArrayObject(const VertexArray &vertex_array) override;

  // shader program
  std::shared_ptr<ShaderProgram> CreateShaderProgram() override;

  // uniform
  std::shared_ptr<UniformBlock> CreateUniformBlock(const std::string &name, int size) override;
  std::shared_ptr<UniformSampler> CreateUniformSampler(const std::string &name) override;

  // pipeline
  void SetFrameBuffer(FrameBuffer &frame_buffer) override;
  void SetViewPort(int x, int y, int width, int height) override;
  void Clear(const ClearState &state) override;
  void SetRenderState(const RenderState &state) override;
  void SetVertexArray(VertexArray &vertex) override;
  void SetShaderProgram(ShaderProgram &program) override;
  void SetShaderUniforms(ShaderUniforms &uniforms) override;
  void Draw(PrimitiveType type) override;

 public:
  void SetDepthRange(float near, float far);

 private:
  DepthRange depth_range_;
  glm::vec4 viewport_;
  FrameBufferSoft *fbo_ = nullptr;
  const RenderState *render_state_ = nullptr;
  VertexArray *vertexArray_ = nullptr;
  ShaderProgramSoft *shader_program_ = nullptr;
};

}
