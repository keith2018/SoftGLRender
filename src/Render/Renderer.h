/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/Buffer.h"
#include "Framebuffer.h"
#include "Texture.h"
#include "RenderState.h"
#include "Vertex.h"
#include "ShaderProgram.h"

namespace SoftGL {

class Renderer {
 public:
  // config reverse z
  virtual void setReverseZ(bool enable) {};
  virtual bool getReverseZ() { return false; };

  // config early z
  virtual void setEarlyZ(bool enable) {};
  virtual bool getEarlyZ() { return false; };

  // framebuffer
  virtual std::shared_ptr<FrameBuffer> createFrameBuffer() = 0;

  // texture
  virtual std::shared_ptr<Texture> createTexture(const TextureDesc &desc) = 0;

  // vertex
  virtual std::shared_ptr<VertexArrayObject> createVertexArrayObject(const VertexArray &vertexArray) = 0;

  // shader program
  virtual std::shared_ptr<ShaderProgram> createShaderProgram() = 0;

  // uniform
  virtual std::shared_ptr<UniformBlock> createUniformBlock(const std::string &name, int size) = 0;
  virtual std::shared_ptr<UniformSampler> createUniformSampler(const std::string &name, TextureType type,
                                                               TextureFormat format) = 0;

  // pipeline
  virtual void setFrameBuffer(std::shared_ptr<FrameBuffer> &frameBuffer) = 0;
  virtual void setViewPort(int x, int y, int width, int height) = 0;
  virtual void clear(const ClearState &state) = 0;
  virtual void setRenderState(const RenderState &state) = 0;
  virtual void setVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) = 0;
  virtual void setShaderProgram(std::shared_ptr<ShaderProgram> &program) = 0;
  virtual void setShaderUniforms(std::shared_ptr<ShaderUniforms> &uniforms) = 0;
  virtual void draw(PrimitiveType type) = 0;
};

}
