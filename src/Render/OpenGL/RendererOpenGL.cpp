/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "RendererOpenGL.h"
#include "FramebufferOpenGL.h"
#include "TextureOpenGL.h"
#include "UniformOpenGL.h"
#include "ShaderProgramOpenGL.h"
#include "VertexOpenGL.h"
#include "EnumsOpenGL.h"

#define GL_STATE_SET(var, gl_state) if (var) GL_CHECK(glEnable(gl_state)); else GL_CHECK(glDisable(gl_state));

namespace SoftGL {

// framebuffer
std::shared_ptr<FrameBuffer> RendererOpenGL::createFrameBuffer() {
  return std::make_shared<FrameBufferOpenGL>();
}

// texture
std::shared_ptr<Texture> RendererOpenGL::createTexture(const TextureDesc &desc) {
  switch (desc.type) {
    case TextureType_2D:    return std::make_shared<Texture2DOpenGL>(desc);
    case TextureType_CUBE:  return std::make_shared<TextureCubeOpenGL>(desc);
  }
  return nullptr;
}

// vertex
std::shared_ptr<VertexArrayObject> RendererOpenGL::createVertexArrayObject(const VertexArray &vertexArray) {
  return std::make_shared<VertexArrayObjectOpenGL>(vertexArray);
}

// shader program
std::shared_ptr<ShaderProgram> RendererOpenGL::createShaderProgram() {
  return std::make_shared<ShaderProgramOpenGL>();
}

// pipeline states
std::shared_ptr<PipelineStates> RendererOpenGL::createPipelineStates(const RenderStates &renderStates) {
  return std::make_shared<PipelineStates>(renderStates);
}

// uniform
std::shared_ptr<UniformBlock> RendererOpenGL::createUniformBlock(const std::string &name, int size) {
  return std::make_shared<UniformBlockOpenGL>(name, size);
}

std::shared_ptr<UniformSampler> RendererOpenGL::createUniformSampler(const std::string &name, const TextureDesc &desc) {
  return std::make_shared<UniformSamplerOpenGL>(name, desc.type, desc.format);
}

// pipeline
void RendererOpenGL::setFrameBuffer(std::shared_ptr<FrameBuffer> &frameBuffer) {
  auto *fbo = dynamic_cast<FrameBufferOpenGL *>(frameBuffer.get());
  fbo->bind();
}

void RendererOpenGL::setViewPort(int x, int y, int width, int height) {
  GL_CHECK(glViewport(x, y, width, height));
}

void RendererOpenGL::clear(const ClearStates &states) {
  GL_CHECK(glClearColor(states.clearColor.r, states.clearColor.g, states.clearColor.b, states.clearColor.a));
  GLbitfield clearBit = 0;
  if (states.colorFlag) {
    clearBit = clearBit | GL_COLOR_BUFFER_BIT;
  }
  if (states.depthFlag) {
    clearBit = clearBit | GL_DEPTH_BUFFER_BIT;
  }
  GL_CHECK(glClear(clearBit));
}

void RendererOpenGL::setVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) {
  if (!vao) {
    return;
  }
  vao_ = dynamic_cast<VertexArrayObjectOpenGL *>(vao.get());
  vao_->bind();
}

void RendererOpenGL::setShaderProgram(std::shared_ptr<ShaderProgram> &program) {
  if (!program) {
    return;
  }
  shaderProgram_ = dynamic_cast<ShaderProgramOpenGL *>(program.get());
  shaderProgram_->use();
}

void RendererOpenGL::setShaderResources(std::shared_ptr<ShaderResources> &resources) {
  if (!resources) {
    return;
  }
  if (shaderProgram_) {
    shaderProgram_->bindResources(*resources);
  }
}

void RendererOpenGL::setPipelineStates(std::shared_ptr<PipelineStates> &states) {
  if (!states) {
    return;
  }
  auto &renderStates = states->renderStates;
  // blend
  GL_STATE_SET(renderStates.blend, GL_BLEND)
  GL_CHECK(glBlendEquationSeparate(OpenGL::cvtBlendFunction(renderStates.blendParams.blendFuncRgb),
                                   OpenGL::cvtBlendFunction(renderStates.blendParams.blendFuncAlpha)));
  GL_CHECK(glBlendFuncSeparate(OpenGL::cvtBlendFactor(renderStates.blendParams.blendSrcRgb),
                               OpenGL::cvtBlendFactor(renderStates.blendParams.blendDstRgb),
                               OpenGL::cvtBlendFactor(renderStates.blendParams.blendSrcAlpha),
                               OpenGL::cvtBlendFactor(renderStates.blendParams.blendDstAlpha)));

  // depth
  GL_STATE_SET(renderStates.depthTest, GL_DEPTH_TEST)
  GL_CHECK(glDepthMask(renderStates.depthMask));
  GL_CHECK(glDepthFunc(OpenGL::cvtDepthFunc(renderStates.depthFunc)));

  GL_STATE_SET(renderStates.cullFace, GL_CULL_FACE)
  GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, OpenGL::cvtPolygonMode(renderStates.polygonMode)));

  GL_CHECK(glLineWidth(renderStates.lineWidth));
  GL_CHECK(glPointSize(renderStates.pointSize));
}

void RendererOpenGL::draw(PrimitiveType type) {
  GLenum mode = OpenGL::cvtDrawMode(type);
  GL_CHECK(glDrawElements(mode, (GLsizei) vao_->getIndicesCnt(), GL_UNSIGNED_INT, nullptr));
}

}
