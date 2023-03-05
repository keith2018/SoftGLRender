/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/buffer.h"
#include "framebuffer.h"
#include "texture.h"
#include "render_state.h"
#include "vertex.h"
#include "shader_program.h"

namespace SoftGL {

class Renderer {
 public:
  // config
  virtual void SetReverseZ(bool enable) = 0;
  virtual void SetEarlyZ(bool enable) = 0;

  // framebuffer
  virtual std::shared_ptr<FrameBuffer> CreateFrameBuffer() = 0;

  // texture
  virtual std::shared_ptr<Texture> CreateTexture(const TextureDesc &desc) = 0;

  // vertex
  virtual std::shared_ptr<VertexArrayObject> CreateVertexArrayObject(const VertexArray &vertex_array) = 0;

  // shader program
  virtual std::shared_ptr<ShaderProgram> CreateShaderProgram() = 0;

  // uniform
  virtual std::shared_ptr<UniformBlock> CreateUniformBlock(const std::string &name, int size) = 0;
  virtual std::shared_ptr<UniformSampler> CreateUniformSampler(const std::string &name,
                                                               TextureType type,
                                                               TextureFormat format) = 0;

  // pipeline
  virtual void SetFrameBuffer(std::shared_ptr<FrameBuffer> &frame_buffer) = 0;
  virtual void SetViewPort(int x, int y, int width, int height) = 0;
  virtual void Clear(const ClearState &state) = 0;
  virtual void SetRenderState(const RenderState &state) = 0;
  virtual void SetVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) = 0;
  virtual void SetShaderProgram(std::shared_ptr<ShaderProgram> &program) = 0;
  virtual void SetShaderUniforms(std::shared_ptr<ShaderUniforms> &uniforms) = 0;
  virtual void Draw(PrimitiveType type) = 0;
};

}
