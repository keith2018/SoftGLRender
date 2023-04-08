/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Render/Renderer.h"
#include "Render/OpenGL/VertexOpenGL.h"
#include "Render/OpenGL/ShaderProgramOpenGL.h"

namespace SoftGL {

class RendererOpenGL : public Renderer {
 public:
  // framebuffer
  std::shared_ptr<FrameBuffer> createFrameBuffer(bool offscreen) override;

  // texture
  std::shared_ptr<Texture> createTexture(const TextureDesc &desc) override;

  // vertex
  std::shared_ptr<VertexArrayObject> createVertexArrayObject(const VertexArray &vertexArray) override;

  // shader program
  std::shared_ptr<ShaderProgram> createShaderProgram() override;

  // pipeline states
  std::shared_ptr<PipelineStates> createPipelineStates(const RenderStates &renderStates) override;

  // uniform
  std::shared_ptr<UniformBlock> createUniformBlock(const std::string &name, int size) override;
  std::shared_ptr<UniformSampler> createUniformSampler(const std::string &name, const TextureDesc &desc) override;

  // pipeline
  void beginRenderPass(std::shared_ptr<FrameBuffer> &frameBuffer, const ClearStates &states) override;
  void setViewPort(int x, int y, int width, int height) override;
  void setVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) override;
  void setShaderProgram(std::shared_ptr<ShaderProgram> &program) override;
  void setShaderResources(std::shared_ptr<ShaderResources> &resources) override;
  void setPipelineStates(std::shared_ptr<PipelineStates> &states) override;
  void draw() override;
  void endRenderPass() override;
  void waitIdle() override;

 private:
  VertexArrayObjectOpenGL *vao_ = nullptr;
  ShaderProgramOpenGL *shaderProgram_ = nullptr;
  PipelineStates *pipelineStates_ = nullptr;
};

}
