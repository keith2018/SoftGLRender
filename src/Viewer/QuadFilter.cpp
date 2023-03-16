/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "QuadFilter.h"

namespace SoftGL {
namespace View {

QuadFilter::QuadFilter(int width, int height, const std::shared_ptr<Renderer> &renderer,
                       const std::function<bool(ShaderProgram &program)> &shaderFunc) {
  if (!renderer) {
    LOGE("QuadFilter error: renderer nullptr");
    return;
  }
  width_ = width;
  height_ = height;

  // quad mesh
  quadMesh_.primitiveType = Primitive_TRIANGLE;
  quadMesh_.primitiveCnt = 2;
  quadMesh_.vertexes.push_back({{1.f, -1.f, 0.f}, {1.f, 0.f}});
  quadMesh_.vertexes.push_back({{-1.f, -1.f, 0.f}, {0.f, 0.f}});
  quadMesh_.vertexes.push_back({{1.f, 1.f, 0.f}, {1.f, 1.f}});
  quadMesh_.vertexes.push_back({{-1.f, 1.f, 0.f}, {0.f, 1.f}});
  quadMesh_.indices = {0, 1, 2, 1, 2, 3};
  quadMesh_.InitVertexes();

  // renderer
  renderer_ = renderer;

  // fbo
  fbo_ = renderer_->createFrameBuffer();

  // vao
  quadMesh_.vao = renderer_->createVertexArrayObject(quadMesh_);

  // program
  auto program = renderer_->createShaderProgram();
  bool success = shaderFunc(*program);
  if (!success) {
    LOGE("create shader program failed");
    return;
  }
  quadMesh_.materialTextured.shaderProgram = program;
  quadMesh_.materialTextured.shaderUniforms = std::make_shared<ShaderUniforms>();

  // uniforms
  TextureUsage usage = TextureUsage_QUAD_FILTER;
  const char *samplerName = Material::samplerName(usage);
  uniformTexIn_ =
      renderer_->createUniformSampler(samplerName, {width_, height_, TextureType_2D, TextureFormat_RGBA8, false});
  quadMesh_.materialTextured.shaderUniforms->samplers[usage] = uniformTexIn_;

  uniformBlockFilter_ = renderer_->createUniformBlock("UniformsQuadFilter", sizeof(UniformsQuadFilter));
  uniformBlockFilter_->setData(&uniformFilter_, sizeof(UniformsQuadFilter));
  quadMesh_.materialTextured.shaderUniforms->blocks[UniformBlock_QuadFilter] = uniformBlockFilter_;

  initReady_ = true;
}

void QuadFilter::setTextures(std::shared_ptr<Texture> &texIn, std::shared_ptr<Texture> &texOut) {
  if (width_ != texIn->width || height_ != texIn->height) {
    LOGE("setTextures failed, texIn size not match");
    return;
  }

  if (width_ != texOut->width || height_ != texOut->height) {
    LOGE("setTextures failed, texOut size not match");
    return;
  }

  uniformFilter_.u_screenSize = glm::vec2(width_, height_);
  uniformBlockFilter_->setData(&uniformFilter_, sizeof(UniformsQuadFilter));

  uniformTexIn_->setTexture(texIn);
  fbo_->setColorAttachment(texOut, 0);
}

void QuadFilter::draw() {
  if (!initReady_) {
    return;
  }

  renderer_->setFrameBuffer(fbo_);
  renderer_->setViewPort(0, 0, width_, height_);

  renderer_->clear({});
  renderer_->setVertexArrayObject(quadMesh_.vao);
  renderer_->setRenderState(quadMesh_.materialTextured.renderState);
  renderer_->setShaderProgram(quadMesh_.materialTextured.shaderProgram);
  renderer_->setShaderUniforms(quadMesh_.materialTextured.shaderUniforms);
  renderer_->draw(quadMesh_.primitiveType);
}

}
}
