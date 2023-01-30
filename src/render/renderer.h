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
  // framebuffer
  virtual std::shared_ptr<FrameBuffer> CreateFrameBuffer() = 0;

  // texture
  virtual std::shared_ptr<Texture2D> CreateTexture2DRef(int refId) = 0;
  virtual std::shared_ptr<Texture2D> CreateTexture2D() = 0;
  virtual std::shared_ptr<TextureCube> CreateTextureCube() = 0;
  virtual std::shared_ptr<TextureDepth> CreateTextureDepth() = 0;

  // vertex
  virtual std::shared_ptr<VertexArrayObject> CreateVertexArrayObject(const VertexArray &vertex_array) = 0;

  // shader program
  virtual std::shared_ptr<ShaderProgram> CreateShaderProgram() = 0;

  // uniform
  virtual std::shared_ptr<UniformBlock> CreateUniformBlock(const std::string &name, int size) = 0;
  virtual std::shared_ptr<UniformSampler> CreateUniformSampler(const std::string &name) = 0;

  // pipeline
  virtual void SetFrameBuffer(FrameBuffer &frame_buffer) = 0;
  virtual void SetViewPort(int x, int y, int width, int height) = 0;
  virtual void Clear(const ClearState &state) = 0;
  virtual void SetRenderState(const RenderState &state) = 0;
  virtual void SetVertexArray(VertexArray &vertex) = 0;
  virtual void SetShaderProgram(ShaderProgram &program, ProgramUniforms &uniforms) = 0;
  virtual void Draw(PrimitiveType type) = 0;
};

}
