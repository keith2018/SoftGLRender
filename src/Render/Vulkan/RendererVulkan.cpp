/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "RendererVulkan.h"
#include "Base/Logger.h"
#include "vulkan/vulkan.h"

namespace SoftGL {

RendererVulkan::RendererVulkan() {
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  LOGD("%d vulkan extensions supported", extensionCount);
}

// framebuffer
std::shared_ptr<FrameBuffer> RendererVulkan::createFrameBuffer() {
  return nullptr;
}

// texture
std::shared_ptr<Texture> RendererVulkan::createTexture(const TextureDesc &desc) {
  return nullptr;
}

// vertex
std::shared_ptr<VertexArrayObject> RendererVulkan::createVertexArrayObject(const VertexArray &vertexArray) {
  return nullptr;
}

// shader program
std::shared_ptr<ShaderProgram> RendererVulkan::createShaderProgram() {
  return nullptr;
}

// uniform
std::shared_ptr<UniformBlock> RendererVulkan::createUniformBlock(const std::string &name, int size) {
  return nullptr;
}

std::shared_ptr<UniformSampler> RendererVulkan::createUniformSampler(const std::string &name, TextureType type,
                                                                     TextureFormat format) {
  return nullptr;
}

// pipeline
void RendererVulkan::setFrameBuffer(std::shared_ptr<FrameBuffer> &frameBuffer) {
}

void RendererVulkan::setViewPort(int x, int y, int width, int height) {
}

void RendererVulkan::clear(const ClearState &state) {
}

void RendererVulkan::setRenderState(const RenderState &state) {
}

void RendererVulkan::setVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) {
  if (!vao) {
    return;
  }
}

void RendererVulkan::setShaderProgram(std::shared_ptr<ShaderProgram> &program) {
  if (!program) {
    return;
  }
}

void RendererVulkan::setShaderUniforms(std::shared_ptr<ShaderUniforms> &uniforms) {
  if (!uniforms) {
    return;
  }
}

void RendererVulkan::draw(PrimitiveType type) {
}

}
