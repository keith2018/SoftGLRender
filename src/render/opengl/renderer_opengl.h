/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/renderer.h"
#include "render/opengl/vertex_opengl.h"
#include "render/opengl/shader_program_opengl.h"

namespace SoftGL {

class RendererOpenGL : public Renderer {
 public:
  // framebuffer
  std::shared_ptr<FrameBuffer> createFrameBuffer() override;

  // texture
  std::shared_ptr<Texture> createTexture(const TextureDesc &desc) override;

  // vertex
  std::shared_ptr<VertexArrayObject> createVertexArrayObject(const VertexArray &vertexArray) override;

  // shader program
  std::shared_ptr<ShaderProgram> createShaderProgram() override;

  // uniform
  std::shared_ptr<UniformBlock> createUniformBlock(const std::string &name, int size) override;
  std::shared_ptr<UniformSampler> createUniformSampler(const std::string &name, TextureType type,
                                                       TextureFormat format) override;

  // pipeline
  void setFrameBuffer(std::shared_ptr<FrameBuffer> &frameBuffer) override;
  void setViewPort(int x, int y, int width, int height) override;
  void clear(const ClearState &state) override;
  void setRenderState(const RenderState &state) override;
  void setVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) override;
  void setShaderProgram(std::shared_ptr<ShaderProgram> &program) override;
  void setShaderUniforms(std::shared_ptr<ShaderUniforms> &uniforms) override;
  void draw(PrimitiveType type) override;

 private:
  VertexArrayObjectOpenGL *vao_ = nullptr;
  ShaderProgramOpenGL *shaderProgram_ = nullptr;
};

}
