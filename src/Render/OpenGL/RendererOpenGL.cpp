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

// uniform
std::shared_ptr<UniformBlock> RendererOpenGL::createUniformBlock(const std::string &name, int size) {
  return std::make_shared<UniformBlockOpenGL>(name, size);
}

std::shared_ptr<UniformSampler> RendererOpenGL::createUniformSampler(const std::string &name, TextureType type,
                                                                     TextureFormat format) {
  return std::make_shared<UniformSamplerOpenGL>(name, type, format);
}

// pipeline
void RendererOpenGL::setFrameBuffer(std::shared_ptr<FrameBuffer> &frameBuffer) {
  auto *fbo = dynamic_cast<FrameBufferOpenGL *>(frameBuffer.get());
  fbo->bind();
}

void RendererOpenGL::setViewPort(int x, int y, int width, int height) {
  GL_CHECK(glViewport(x, y, width, height));
}

void RendererOpenGL::clear(const ClearState &state) {
  GL_CHECK(glClearColor(state.clearColor.r, state.clearColor.g, state.clearColor.b, state.clearColor.a));
  GLbitfield clearBit = 0;
  if (state.colorFlag) {
    clearBit = clearBit | GL_COLOR_BUFFER_BIT;
  }
  if (state.depthFlag) {
    clearBit = clearBit | GL_DEPTH_BUFFER_BIT;
  }
  GL_CHECK(glClear(clearBit));
}

void RendererOpenGL::setRenderState(const RenderState &state) {
  // blend
  GL_STATE_SET(state.blend, GL_BLEND)
  GL_CHECK(glBlendEquationSeparate(OpenGL::convertBlendFunction(state.blendParams.blendFuncRgb),
                                   OpenGL::convertBlendFunction(state.blendParams.blendFuncAlpha)));
  GL_CHECK(glBlendFuncSeparate(OpenGL::convertBlendFactor(state.blendParams.blendSrcRgb),
                               OpenGL::convertBlendFactor(state.blendParams.blendDstRgb),
                               OpenGL::convertBlendFactor(state.blendParams.blendSrcAlpha),
                               OpenGL::convertBlendFactor(state.blendParams.blendDstAlpha)));

  // depth
  GL_STATE_SET(state.depthTest, GL_DEPTH_TEST)
  GL_CHECK(glDepthMask(state.depthMask));
  GL_CHECK(glDepthFunc(OpenGL::convertDepthFunc(state.depthFunc)));

  GL_STATE_SET(state.cullFace, GL_CULL_FACE)
  GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, OpenGL::convertPolygonMode(state.polygonMode)));

  GL_CHECK(glLineWidth(state.lineWidth));
  GL_CHECK(glPointSize(state.pointSize));
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

void RendererOpenGL::setShaderUniforms(std::shared_ptr<ShaderUniforms> &uniforms) {
  if (!uniforms) {
    return;
  }
  if (shaderProgram_) {
    shaderProgram_->bindUniforms(*uniforms);
  }
}

void RendererOpenGL::draw(PrimitiveType type) {
  GLenum mode = OpenGL::convertDrawMode(type);
  GL_CHECK(glDrawElements(mode, (GLsizei) vao_->getIndicesCnt(), GL_UNSIGNED_INT, nullptr));
}

}
