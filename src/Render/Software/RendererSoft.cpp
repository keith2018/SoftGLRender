/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "RendererSoft.h"
#include "Base/SIMD.h"
#include "Base/HashUtils.h"
#include "FramebufferSoft.h"
#include "TextureSoft.h"
#include "UniformSoft.h"
#include "ShaderProgramSoft.h"
#include "VertexSoft.h"
#include "BlendSoft.h"
#include "DepthSoft.h"

namespace SoftGL {

#define RASTER_MULTI_THREAD

// framebuffer
std::shared_ptr<FrameBuffer> RendererSoft::createFrameBuffer(bool offscreen) {
  return std::make_shared<FrameBufferSoft>(offscreen);
}

// texture
std::shared_ptr<Texture> RendererSoft::createTexture(const TextureDesc &desc) {
  switch (desc.type) {
    case TextureType_2D:
      switch (desc.format) {
        case TextureFormat_RGBA8:     return std::make_shared<Texture2DSoft<RGBA>>(desc);
        case TextureFormat_FLOAT32:   return std::make_shared<Texture2DSoft<float>>(desc);
      }
    case TextureType_CUBE:
      switch (desc.format) {
        case TextureFormat_RGBA8:     return std::make_shared<TextureCubeSoft<RGBA>>(desc);
        case TextureFormat_FLOAT32:   return std::make_shared<TextureCubeSoft<float>>(desc);
      }
  }
  return nullptr;
}

// vertex
std::shared_ptr<VertexArrayObject> RendererSoft::createVertexArrayObject(const VertexArray &vertexArray) {
  return std::make_shared<VertexArrayObjectSoft>(vertexArray);
}

// shader program
std::shared_ptr<ShaderProgram> RendererSoft::createShaderProgram() {
  return std::make_shared<ShaderProgramSoft>();
}

// pipeline states
std::shared_ptr<PipelineStates> RendererSoft::createPipelineStates(const RenderStates &renderStates) {
  return std::make_shared<PipelineStates>(renderStates);
}

// uniform
std::shared_ptr<UniformBlock> RendererSoft::createUniformBlock(const std::string &name, int size) {
  return std::make_shared<UniformBlockSoft>(name, size);
}

std::shared_ptr<UniformSampler> RendererSoft::createUniformSampler(const std::string &name, const TextureDesc &desc) {
  return std::make_shared<UniformSamplerSoft>(name, desc.type, desc.format);
}

// pipeline
void RendererSoft::beginRenderPass(std::shared_ptr<FrameBuffer> &frameBuffer, const ClearStates &states) {
  fbo_ = dynamic_cast<FrameBufferSoft *>(frameBuffer.get());

  if (!fbo_) {
    return;
  }

  fboColor_ = fbo_->getColorBuffer();
  fboDepth_ = fbo_->getDepthBuffer();

  if (states.colorFlag && fboColor_) {
    RGBA color = RGBA(states.clearColor.r * 255,
                      states.clearColor.g * 255,
                      states.clearColor.b * 255,
                      states.clearColor.a * 255);
    if (fboColor_->multiSample) {
      fboColor_->bufferMs4x->setAll(glm::tvec4<RGBA>(color));
    } else {
      fboColor_->buffer->setAll(color);
    }
  }

  if (states.depthFlag && fboDepth_) {
    if (fboDepth_->multiSample) {
      fboDepth_->bufferMs4x->setAll(glm::tvec4<float>(states.clearDepth));
    } else {
      fboDepth_->buffer->setAll(states.clearDepth);
    }
  }
}

void RendererSoft::setViewPort(int x, int y, int width, int height) {
  viewport_.x = (float) x;
  viewport_.y = (float) y;
  viewport_.width = (float) width;
  viewport_.height = (float) height;

  viewport_.minDepth = 0.f;
  viewport_.maxDepth = 1.f;

  viewport_.absMinDepth = std::min(viewport_.minDepth, viewport_.maxDepth);
  viewport_.absMaxDepth = std::max(viewport_.minDepth, viewport_.maxDepth);

  viewport_.innerO.x = viewport_.x + viewport_.width / 2.f;
  viewport_.innerO.y = viewport_.y + viewport_.height / 2.f;
  viewport_.innerO.z = viewport_.minDepth;
  viewport_.innerO.w = 0.f;

  viewport_.innerP.x = viewport_.width / 2.f;    // divide by 2 in advance
  viewport_.innerP.y = viewport_.height / 2.f;   // divide by 2 in advance
  viewport_.innerP.z = viewport_.maxDepth - viewport_.minDepth;
  viewport_.innerP.w = 1.f;
}

void RendererSoft::setVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) {
  vao_ = dynamic_cast<VertexArrayObjectSoft *>(vao.get());
}

void RendererSoft::setShaderProgram(std::shared_ptr<ShaderProgram> &program) {
  shaderProgram_ = dynamic_cast<ShaderProgramSoft *>(program.get());
}

void RendererSoft::setShaderResources(std::shared_ptr<ShaderResources> &resources) {
  if (!resources) {
    return;
  }
  if (shaderProgram_) {
    shaderProgram_->bindResources(*resources);
  }
}

void RendererSoft::setPipelineStates(std::shared_ptr<PipelineStates> &states) {
  renderState_ = &states->renderStates;
}

void RendererSoft::draw() {
  if (!fbo_ || !vao_ || !shaderProgram_) {
    return;
  }

  fboColor_ = fbo_->getColorBuffer();
  fboDepth_ = fbo_->getDepthBuffer();
  primitiveType_ = renderState_->primitiveType;

  if (fboColor_) {
    rasterSamples_ = fboColor_->sampleCnt;
  } else if (fboDepth_) {
    rasterSamples_ = fboDepth_->sampleCnt;
  } else {
    rasterSamples_ = 1;
  }

  processVertexShader();
  processPrimitiveAssembly();
  processClipping();
  processPerspectiveDivide();
  processViewportTransform();
  processFaceCulling();
  processRasterization();

  if (fboColor_ && fboColor_->multiSample) {
    multiSampleResolve();
  }
}

void RendererSoft::endRenderPass() {}

void RendererSoft::waitIdle() {}

void RendererSoft::processVertexShader() {
  // init shader varyings
  varyingsCnt_ = shaderProgram_->getShaderVaryingsSize() / sizeof(float);
  varyingsAlignedSize_ = MemoryUtils::alignedSize(varyingsCnt_ * sizeof(float));
  varyingsAlignedCnt_ = varyingsAlignedSize_ / sizeof(float);

  varyings_ = MemoryUtils::makeAlignedBuffer<float>(vao_->vertexCnt * varyingsAlignedCnt_);
  float *varyingBuffer = varyings_.get();

  uint8_t *vertexPtr = vao_->vertexes.data();
  vertexes_.resize(vao_->vertexCnt);
  for (int idx = 0; idx < vao_->vertexCnt; idx++) {
    VertexHolder &holder = vertexes_[idx];
    holder.discard = false;
    holder.index = idx;
    holder.vertex = vertexPtr;
    holder.varyings = (varyingsAlignedSize_ > 0) ? (varyingBuffer + idx * varyingsAlignedCnt_) : nullptr;
    vertexShaderImpl(holder);
    vertexPtr += vao_->vertexStride;
  }
}

void RendererSoft::processPrimitiveAssembly() {
  switch (primitiveType_) {
    case Primitive_POINT:
      processPointAssembly();
      break;
    case Primitive_LINE:
      processLineAssembly();
      break;
    case Primitive_TRIANGLE:
      processPolygonAssembly();
      break;
  }
}

void RendererSoft::processClipping() {
  size_t primitiveCnt = primitives_.size();
  for (int i = 0; i < primitiveCnt; i++) {
    auto &primitive = primitives_[i];
    if (primitive.discard) {
      continue;
    }
    switch (primitiveType_) {
      case Primitive_POINT:
        clippingPoint(primitive);
        break;
      case Primitive_LINE:
        clippingLine(primitive);
        break;
      case Primitive_TRIANGLE:
        // skip clipping if draw triangles with point/line mode
        if (renderState_->polygonMode != PolygonMode_FILL) {
          continue;
        }
        clippingTriangle(primitive);
        break;
    }
  }

  // set all vertexes discard flag to true
  for (auto &vertex : vertexes_) {
    vertex.discard = true;
  }

  // set discard flag to false base on primitive discard flag
  for (auto &primitive : primitives_) {
    if (primitive.discard) {
      continue;
    }
    switch (primitiveType_) {
      case Primitive_POINT:
        vertexes_[primitive.indices[0]].discard = false;
        break;
      case Primitive_LINE:
        vertexes_[primitive.indices[0]].discard = false;
        vertexes_[primitive.indices[1]].discard = false;
        break;
      case Primitive_TRIANGLE:
        vertexes_[primitive.indices[0]].discard = false;
        vertexes_[primitive.indices[1]].discard = false;
        vertexes_[primitive.indices[2]].discard = false;
        break;
    }
  }
}

void RendererSoft::processPerspectiveDivide() {
  for (auto &vertex : vertexes_) {
    if (vertex.discard) {
      continue;
    }
    perspectiveDivideImpl(vertex);
  }
}

void RendererSoft::processViewportTransform() {
  for (auto &vertex : vertexes_) {
    if (vertex.discard) {
      continue;
    }
    viewportTransformImpl(vertex);
  }
}

void RendererSoft::processFaceCulling() {
  if (primitiveType_ != Primitive_TRIANGLE) {
    return;
  }

  for (auto &triangle : primitives_) {
    if (triangle.discard) {
      continue;
    }

    glm::vec4 &v0 = vertexes_[triangle.indices[0]].fragPos;
    glm::vec4 &v1 = vertexes_[triangle.indices[1]].fragPos;
    glm::vec4 &v2 = vertexes_[triangle.indices[2]].fragPos;

    glm::vec3 n = glm::cross(glm::vec3(v1 - v0), glm::vec3(v2 - v0));
    float area = glm::dot(n, glm::vec3(0, 0, 1));
    triangle.frontFacing = area > 0;

    if (renderState_->cullFace) {
      triangle.discard = !triangle.frontFacing;  // discard back face
    }
  }
}

void RendererSoft::processRasterization() {
  switch (primitiveType_) {
    case Primitive_POINT:
      for (auto &primitive : primitives_) {
        if (primitive.discard) {
          continue;
        }
        auto *vert0 = &vertexes_[primitive.indices[0]];
        rasterizationPoint(vert0, pointSize_);
      }
      break;
    case Primitive_LINE:
      for (auto &primitive : primitives_) {
        if (primitive.discard) {
          continue;
        }
        auto *vert0 = &vertexes_[primitive.indices[0]];
        auto *vert1 = &vertexes_[primitive.indices[1]];
        rasterizationLine(vert0, vert1, renderState_->lineWidth);
      }
      break;
    case Primitive_TRIANGLE:
      threadQuadCtx_.resize(threadPool_.getThreadCnt());
      for (auto &ctx : threadQuadCtx_) {
        ctx.SetVaryingsSize(varyingsAlignedCnt_);
        ctx.shaderProgram = shaderProgram_->clone();
        ctx.shaderProgram->prepareFragmentShader();

        // setup derivative
        DerivativeContext &df_ctx = ctx.shaderProgram->getShaderBuiltin().dfCtx;
        df_ctx.p0 = ctx.pixels[0].varyingsFrag;
        df_ctx.p1 = ctx.pixels[1].varyingsFrag;
        df_ctx.p2 = ctx.pixels[2].varyingsFrag;
        df_ctx.p3 = ctx.pixels[3].varyingsFrag;
      }
      rasterizationPolygons(primitives_);
      threadPool_.waitTasksFinish();
      break;
  }
}

void RendererSoft::processFragmentShader(glm::vec4 &screenPos,
                                         bool front_facing,
                                         void *varyings,
                                         ShaderProgramSoft *shader) {
  if (!fboColor_) {
    return;
  }

  auto &builtin = shader->getShaderBuiltin();
  builtin.FragCoord = screenPos;
  builtin.FrontFacing = front_facing;

  shader->bindFragmentShaderVaryings(varyings);
  shader->execFragmentShader();
}

void RendererSoft::processPerSampleOperations(int x, int y, float depth, const glm::vec4 &color, int sample) {
  // depth test
  if (!processDepthTest(x, y, depth, sample, false)) {
    return;
  }

  if (!fboColor_) {
    return;
  }

  glm::vec4 color_clamp = glm::clamp(color, 0.f, 1.f);

  // color blending
  processColorBlending(x, y, color_clamp, sample);

  // write final color to fbo
  setFrameColor(x, y, color_clamp * 255.f, sample);
}

bool RendererSoft::processDepthTest(int x, int y, float depth, int sample, bool skipWrite) {
  if (!renderState_->depthTest || !fboDepth_) {
    return true;
  }

  // depth clamping
  depth = glm::clamp(depth, viewport_.absMinDepth, viewport_.absMaxDepth);

  // depth comparison
  float *zPtr = getFrameDepth(x, y, sample);
  if (zPtr && DepthTest(depth, *zPtr, renderState_->depthFunc)) {
    // depth attachment writes
    if (!skipWrite && renderState_->depthMask) {
      *zPtr = depth;
    }
    return true;
  }
  return false;
}

void RendererSoft::processColorBlending(int x, int y, glm::vec4 &color, int sample) {
  if (renderState_->blend) {
    glm::vec4 &srcColor = color;
    glm::vec4 dstColor = glm::vec4(0.f);
    auto *ptr = getFrameColor(x, y, sample);
    if (ptr) {
      dstColor = glm::vec4(*ptr) / 255.f;
    }
    color = calcBlendColor(srcColor, dstColor, renderState_->blendParams);
  }
}

void RendererSoft::processPointAssembly() {
  primitives_.resize(vao_->indicesCnt);
  for (int idx = 0; idx < primitives_.size(); idx++) {
    auto &point = primitives_[idx];
    point.indices[0] = vao_->indices[idx];
    point.discard = false;
  }
}

void RendererSoft::processLineAssembly() {
  primitives_.resize(vao_->indicesCnt / 2);
  for (int idx = 0; idx < primitives_.size(); idx++) {
    auto &line = primitives_[idx];
    line.indices[0] = vao_->indices[idx * 2];
    line.indices[1] = vao_->indices[idx * 2 + 1];
    line.discard = false;
  }
}

void RendererSoft::processPolygonAssembly() {
  primitives_.resize(vao_->indicesCnt / 3);
  for (int idx = 0; idx < primitives_.size(); idx++) {
    auto &triangle = primitives_[idx];
    triangle.indices[0] = vao_->indices[idx * 3];
    triangle.indices[1] = vao_->indices[idx * 3 + 1];
    triangle.indices[2] = vao_->indices[idx * 3 + 2];
    triangle.discard = false;
  }
}

void RendererSoft::clippingPoint(PrimitiveHolder &point) {
  point.discard = (vertexes_[point.indices[0]].clipMask != 0);
}

void RendererSoft::clippingLine(PrimitiveHolder &line, bool postVertexProcess) {
  VertexHolder *v0 = &vertexes_[line.indices[0]];
  VertexHolder *v1 = &vertexes_[line.indices[1]];

  bool fullClip = false;
  float t0 = 0.f;
  float t1 = 1.f;

  int mask = v0->clipMask | v1->clipMask;
  if (mask != 0) {
    for (int i = 0; i < 6; i++) {
      if (mask & FrustumClipMaskArray[i]) {
        float d0 = glm::dot(FrustumClipPlane[i], v0->clipPos);
        float d1 = glm::dot(FrustumClipPlane[i], v1->clipPos);

        if (d0 < 0 && d1 < 0) {
          fullClip = true;
          break;
        } else if (d0 < 0) {
          float t = -d0 / (d1 - d0);
          t0 = std::max(t0, t);
        } else {
          float t = d0 / (d0 - d1);
          t1 = std::min(t1, t);
        }
      }
    }
  }

  if (fullClip) {
    line.discard = true;
    return;
  }

  if (v0->clipMask) {
    auto &vert = clippingNewVertex(line.indices[0], line.indices[1], t0, postVertexProcess);
    line.indices[0] = vert.index;
  }

  if (v1->clipMask) {
    auto &vert = clippingNewVertex(line.indices[0], line.indices[1], t1, postVertexProcess);
    line.indices[1] = vert.index;
  }
}

void RendererSoft::clippingTriangle(PrimitiveHolder &triangle) {
  auto *v0 = &vertexes_[triangle.indices[0]];
  auto *v1 = &vertexes_[triangle.indices[1]];
  auto *v2 = &vertexes_[triangle.indices[2]];

  int mask = v0->clipMask | v1->clipMask | v2->clipMask;
  if (mask == 0) {
    return;
  }

  bool fullClip = false;
  std::vector<size_t> indicesIn;
  std::vector<size_t> indicesOut;

  indicesIn.push_back(v0->index);
  indicesIn.push_back(v1->index);
  indicesIn.push_back(v2->index);

  for (int planeIdx = 0; planeIdx < 6; planeIdx++) {
    if (mask & FrustumClipMaskArray[planeIdx]) {
      if (indicesIn.size() < 3) {
        fullClip = true;
        break;
      }
      indicesOut.clear();
      size_t idxPre = indicesIn[0];
      float dPre = glm::dot(FrustumClipPlane[planeIdx], vertexes_[idxPre].clipPos);

      indicesIn.push_back(idxPre);
      for (int i = 1; i < indicesIn.size(); i++) {
        size_t idx = indicesIn[i];
        float d = glm::dot(FrustumClipPlane[planeIdx], vertexes_[idx].clipPos);

        if (dPre >= 0) {
          indicesOut.push_back(idxPre);
        }

        if (std::signbit(dPre) != std::signbit(d)) {
          float t = d < 0 ? dPre / (dPre - d) : -dPre / (d - dPre);
          // create new vertex
          auto &vert = clippingNewVertex(idxPre, idx, t);
          indicesOut.push_back(vert.index);
        }

        idxPre = idx;
        dPre = d;
      }

      std::swap(indicesIn, indicesOut);
    }
  }

  if (fullClip || indicesIn.empty()) {
    triangle.discard = true;
    return;
  }

  triangle.indices[0] = indicesIn[0];
  triangle.indices[1] = indicesIn[1];
  triangle.indices[2] = indicesIn[2];

  for (int i = 3; i < indicesIn.size(); i++) {
    primitives_.emplace_back();
    PrimitiveHolder &ph = primitives_.back();
    ph.discard = false;
    ph.indices[0] = indicesIn[0];
    ph.indices[1] = indicesIn[i - 1];
    ph.indices[2] = indicesIn[i];
    ph.frontFacing = triangle.frontFacing;
  }
}

void RendererSoft::rasterizationPolygons(std::vector<PrimitiveHolder> &primitives) {
  switch (renderState_->polygonMode) {
    case PolygonMode_POINT:
      rasterizationPolygonsPoint(primitives);
      break;
    case PolygonMode_LINE:
      rasterizationPolygonsLine(primitives);
      break;
    case PolygonMode_FILL:
      rasterizationPolygonsTriangle(primitives);
      break;
  }
}

void RendererSoft::rasterizationPolygonsPoint(std::vector<PrimitiveHolder> &primitives) {
  for (auto &triangle : primitives) {
    if (triangle.discard) {
      continue;
    }
    for (size_t idx : triangle.indices) {
      PrimitiveHolder point;
      point.discard = false;
      point.frontFacing = triangle.frontFacing;
      point.indices[0] = idx;

      // clipping
      clippingPoint(point);
      if (point.discard) {
        continue;
      }

      // rasterization
      rasterizationPoint(&vertexes_[point.indices[0]], pointSize_);
    }
  }
}

void RendererSoft::rasterizationPolygonsLine(std::vector<PrimitiveHolder> &primitives) {
  for (auto &triangle : primitives) {
    if (triangle.discard) {
      continue;
    }
    for (size_t i = 0; i < 3; i++) {
      PrimitiveHolder line;
      line.discard = false;
      line.frontFacing = triangle.frontFacing;
      line.indices[0] = triangle.indices[i];
      line.indices[1] = triangle.indices[(i + 1) % 3];

      // clipping
      clippingLine(line, true);
      if (line.discard) {
        continue;
      }

      // rasterization
      rasterizationLine(&vertexes_[line.indices[0]],
                        &vertexes_[line.indices[1]],
                        renderState_->lineWidth);
    }
  }
}

void RendererSoft::rasterizationPolygonsTriangle(std::vector<PrimitiveHolder> &primitives) {
  for (auto &triangle : primitives) {
    if (triangle.discard) {
      continue;
    }
    rasterizationTriangle(&vertexes_[triangle.indices[0]],
                          &vertexes_[triangle.indices[1]],
                          &vertexes_[triangle.indices[2]],
                          triangle.frontFacing);
  }
}

void RendererSoft::rasterizationPoint(VertexHolder *v, float pointSize) {
  if (!fboColor_) {
    return;
  }

  float left = v->fragPos.x - pointSize / 2.f + 0.5f;
  float right = left + pointSize;
  float top = v->fragPos.y - pointSize / 2.f + 0.5f;
  float bottom = top + pointSize;

  glm::vec4 &screenPos = v->fragPos;
  for (int x = (int) left; x < (int) right; x++) {
    for (int y = (int) top; y < (int) bottom; y++) {
      screenPos.x = (float) x;
      screenPos.y = (float) y;
      processFragmentShader(screenPos, true, v->varyings, shaderProgram_);
      auto &builtIn = shaderProgram_->getShaderBuiltin();
      if (!builtIn.discard) {
        // TODO MSAA
        for (int idx = 0; idx < rasterSamples_; idx++) {
          processPerSampleOperations(x, y, screenPos.z, builtIn.FragColor, idx);
        }
      }
    }
  }
}

void RendererSoft::rasterizationLine(VertexHolder *v0, VertexHolder *v1, float lineWidth) {
  // TODO diamond-exit rule
  int x0 = (int) v0->fragPos.x, y0 = (int) v0->fragPos.y;
  int x1 = (int) v1->fragPos.x, y1 = (int) v1->fragPos.y;

  float z0 = v0->fragPos.z;
  float z1 = v1->fragPos.z;

  float w0 = v0->fragPos.w;
  float w1 = v1->fragPos.w;

  bool steep = false;
  if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
    std::swap(x0, y0);
    std::swap(x1, y1);
    steep = true;
  }

  const float *varyingsIn[2] = {v0->varyings, v1->varyings};

  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
    std::swap(z0, z1);
    std::swap(w0, w1);
    std::swap(varyingsIn[0], varyingsIn[1]);
  }
  int dx = x1 - x0;
  int dy = y1 - y0;

  int error = 0;
  int dError = 2 * std::abs(dy);

  int y = y0;

  auto varyings = MemoryUtils::makeBuffer<float>(varyingsCnt_);
  VertexHolder pt{};
  pt.varyings = varyings.get();

  float t = 0;
  for (int x = x0; x <= x1; x++) {
    t = (float) (x - x0) / (float) dx;
    pt.fragPos = glm::vec4(x, y, glm::mix(z0, z1, t), glm::mix(w0, w1, t));
    if (steep) {
      std::swap(pt.fragPos.x, pt.fragPos.y);
    }
    interpolateLinear(pt.varyings, varyingsIn, varyingsCnt_, t);
    rasterizationPoint(&pt, lineWidth);

    error += dError;
    if (error > dx) {
      y += (y1 > y0 ? 1 : -1);
      error -= 2 * dx;
    }
  }
}

void RendererSoft::rasterizationTriangle(VertexHolder *v0, VertexHolder *v1, VertexHolder *v2, bool frontFacing) {
  // TODO top-left rule
  VertexHolder *vert[3] = {v0, v1, v2};
  glm::aligned_vec4 screenPos[3] = {vert[0]->fragPos, vert[1]->fragPos, vert[2]->fragPos};
  BoundingBox bounds = triangleBoundingBox(screenPos, viewport_.width, viewport_.height);
  bounds.min -= 1.f;

  auto blockSize = rasterBlockSize_;
  int blockCntX = (bounds.max.x - bounds.min.x + blockSize - 1.f) / blockSize;
  int blockCntY = (bounds.max.y - bounds.min.y + blockSize - 1.f) / blockSize;

  for (int blockY = 0; blockY < blockCntY; blockY++) {
    for (int blockX = 0; blockX < blockCntX; blockX++) {
#ifdef RASTER_MULTI_THREAD
      threadPool_.pushTask([&, vert, bounds, blockSize, blockX, blockY](int thread_id) {
        // init pixel quad
        auto pixelQuad = threadQuadCtx_[thread_id];
#else
        auto pixelQuad = threadQuadCtx_[0];
#endif
        pixelQuad.frontFacing = frontFacing;

        for (int i = 0; i < 3; i++) {
          pixelQuad.vertPos[i] = vert[i]->fragPos;
          pixelQuad.vertZ[i] = &vert[i]->fragPos.z;
          pixelQuad.vertW[i] = vert[i]->fragPos.w;
          pixelQuad.vertVaryings[i] = vert[i]->varyings;
        }

        glm::aligned_vec4 *vertPos = pixelQuad.vertPos;
        pixelQuad.vertPosFlat[0] = {vertPos[2].x, vertPos[1].x, vertPos[0].x, 0.f};
        pixelQuad.vertPosFlat[1] = {vertPos[2].y, vertPos[1].y, vertPos[0].y, 0.f};
        pixelQuad.vertPosFlat[2] = {vertPos[0].z, vertPos[1].z, vertPos[2].z, 0.f};
        pixelQuad.vertPosFlat[3] = {vertPos[0].w, vertPos[1].w, vertPos[2].w, 0.f};

        // block rasterization
        int blockStartX = bounds.min.x + blockX * blockSize;
        int blockStartY = bounds.min.y + blockY * blockSize;
        for (int y = blockStartY + 1; y < blockStartY + blockSize && y <= bounds.max.y; y += 2) {
          for (int x = blockStartX + 1; x < blockStartX + blockSize && x <= bounds.max.x; x += 2) {
            pixelQuad.Init((float) x, (float) y, rasterSamples_);
            rasterizationPixelQuad(pixelQuad);
          }
        }
#ifdef RASTER_MULTI_THREAD
      });
#endif
    }
  }
}

void RendererSoft::rasterizationPixelQuad(PixelQuadContext &quad) {
  glm::aligned_vec4 *vert = quad.vertPosFlat;
  glm::aligned_vec4 &v0 = quad.vertPos[0];

  // barycentric
  for (auto &pixel : quad.pixels) {
    for (auto &sample : pixel.samples) {
      sample.inside = barycentric(vert, v0, sample.position, sample.barycentric);
    }
    pixel.InitCoverage();
    pixel.InitShadingSample();
  }
  if (!quad.CheckInside()) {
    return;
  }

  for (auto &pixel : quad.pixels) {
    for (auto &sample : pixel.samples) {
      if (!sample.inside) {
        continue;
      }

      // interpolate z, w
      interpolateBarycentric(&sample.position.z, quad.vertZ, 2, sample.barycentric);

      // depth clipping
      if (sample.position.z < viewport_.absMinDepth || sample.position.z > viewport_.absMaxDepth) {
        sample.inside = false;
      }

      // barycentric correction
      sample.barycentric *= (1.f / sample.position.w * quad.vertW);
    }
  }

  // early z
  if (earlyZ_ && renderState_->depthTest) {
    if (!earlyZTest(quad)) {
      return;
    }
  }

  // varying interpolate
  // note: all quad pixels should perform varying interpolate to enable varying partial derivative
  for (auto &pixel : quad.pixels) {
    interpolateBarycentric((float *) pixel.varyingsFrag,
                           quad.vertVaryings,
                           varyingsCnt_,
                           pixel.sampleShading->barycentric);
  }

  // pixel shading
  for (auto &pixel : quad.pixels) {
    if (!pixel.inside) {
      continue;
    }

    // fragment shader
    processFragmentShader(pixel.sampleShading->position,
                          quad.frontFacing,
                          pixel.varyingsFrag,
                          quad.shaderProgram.get());

    // sample coverage
    auto &builtIn = quad.shaderProgram->getShaderBuiltin();

    // per-sample operations
    if (pixel.sampleCount > 1) {
      for (int idx = 0; idx < pixel.sampleCount; idx++) {
        auto &sample = pixel.samples[idx];
        if (!sample.inside) {
          continue;
        }
        processPerSampleOperations(sample.fboCoord.x, sample.fboCoord.y, sample.position.z, builtIn.FragColor, idx);
      }
    } else {
      auto &sample = *pixel.sampleShading;
      processPerSampleOperations(sample.fboCoord.x, sample.fboCoord.y, sample.position.z, builtIn.FragColor, 0);
    }
  }
}

bool RendererSoft::earlyZTest(PixelQuadContext &quad) {
  for (auto &pixel : quad.pixels) {
    if (!pixel.inside) {
      continue;
    }
    if (pixel.sampleCount > 1) {
      bool inside = false;
      for (int idx = 0; idx < pixel.sampleCount; idx++) {
        auto &sample = pixel.samples[idx];
        if (!sample.inside) {
          continue;
        }
        sample.inside = processDepthTest(sample.fboCoord.x, sample.fboCoord.y, sample.position.z, idx, true);
        if (sample.inside) {
          inside = true;
        }
      }
      pixel.inside = inside;
    } else {
      auto &sample = *pixel.sampleShading;
      sample.inside = processDepthTest(sample.fboCoord.x, sample.fboCoord.y, sample.position.z, 0, true);
      pixel.inside = sample.inside;
    }
  }
  return quad.CheckInside();
}

void RendererSoft::multiSampleResolve() {
  if (!fboColor_->buffer) {
    fboColor_->buffer = Buffer<RGBA>::makeDefault(fboColor_->width, fboColor_->height);
  }

  auto *srcPtr = fboColor_->bufferMs4x->getRawDataPtr();
  auto *dstPtr = fboColor_->buffer->getRawDataPtr();

  for (size_t row = 0; row < fboColor_->height; row++) {
    auto *rowSrc = srcPtr + row * fboColor_->width;
    auto *rowDst = dstPtr + row * fboColor_->width;
#ifdef RASTER_MULTI_THREAD
    threadPool_.pushTask([&, rowSrc, rowDst](int thread_id) {
#endif
      auto *src = rowSrc;
      auto *dst = rowDst;
      for (size_t idx = 0; idx < fboColor_->width; idx++) {
        glm::vec4 color(0.f);
        for (int i = 0; i < fboColor_->sampleCnt; i++) {
          color += (glm::vec4) (*src)[i];
        }
        color /= fboColor_->sampleCnt;
        *dst = color;
        src++;
        dst++;
      }
#ifdef RASTER_MULTI_THREAD
    });
#endif
  }

  threadPool_.waitTasksFinish();
}

RGBA *RendererSoft::getFrameColor(int x, int y, int sample) {
  if (!fboColor_) {
    return nullptr;
  }

  RGBA *ptr = nullptr;
  if (fboColor_->multiSample) {
    auto *ptrMs = fboColor_->bufferMs4x->get(x, y);
    if (ptrMs) {
      ptr = (RGBA *) ptrMs + sample;
    }
  } else {
    ptr = fboColor_->buffer->get(x, y);
  }

  return ptr;
}

float *RendererSoft::getFrameDepth(int x, int y, int sample) {
  if (!fboDepth_) {
    return nullptr;
  }

  float *depthPtr = nullptr;
  if (fboDepth_->multiSample) {
    auto *ptr = fboDepth_->bufferMs4x->get(x, y);
    if (ptr) {
      depthPtr = &ptr->x + sample;
    }
  } else {
    depthPtr = fboDepth_->buffer->get(x, y);
  }
  return depthPtr;
}

void RendererSoft::setFrameColor(int x, int y, const RGBA &color, int sample) {
  RGBA *ptr = getFrameColor(x, y, sample);
  if (ptr) {
    *ptr = color;
  }
}

VertexHolder &RendererSoft::clippingNewVertex(size_t idx0, size_t idx1, float t, bool postVertexProcess) {
  vertexes_.emplace_back();
  VertexHolder &vh = vertexes_.back();
  vh.discard = false;
  vh.index = vertexes_.size() - 1;
  interpolateVertex(vh, vertexes_[idx0], vertexes_[idx1], t);

  if (postVertexProcess) {
    perspectiveDivideImpl(vh);
    viewportTransformImpl(vh);
  }

  return vh;
}

void RendererSoft::vertexShaderImpl(VertexHolder &vertex) {
  shaderProgram_->bindVertexAttributes(vertex.vertex);
  shaderProgram_->bindVertexShaderVaryings(vertex.varyings);
  shaderProgram_->execVertexShader();

  pointSize_ = shaderProgram_->getShaderBuiltin().PointSize;
  vertex.clipPos = shaderProgram_->getShaderBuiltin().Position;
  vertex.clipMask = countFrustumClipMask(vertex.clipPos);
}

void RendererSoft::perspectiveDivideImpl(VertexHolder &vertex) {
  vertex.fragPos = vertex.clipPos;
  auto &pos = vertex.fragPos;
  float invW = 1.f / pos.w;
  pos *= invW;
  pos.w = invW;
}

void RendererSoft::viewportTransformImpl(VertexHolder &vertex) {
  vertex.fragPos *= viewport_.innerP;
  vertex.fragPos += viewport_.innerO;
}

int RendererSoft::countFrustumClipMask(glm::vec4 &clipPos) {
  int mask = 0;
  if (clipPos.w < clipPos.x) mask |= FrustumClipMask::POSITIVE_X;
  if (clipPos.w < -clipPos.x) mask |= FrustumClipMask::NEGATIVE_X;
  if (clipPos.w < clipPos.y) mask |= FrustumClipMask::POSITIVE_Y;
  if (clipPos.w < -clipPos.y) mask |= FrustumClipMask::NEGATIVE_Y;
  if (clipPos.w < clipPos.z) mask |= FrustumClipMask::POSITIVE_Z;
  if (clipPos.w < -clipPos.z) mask |= FrustumClipMask::NEGATIVE_Z;
  return mask;
}

BoundingBox RendererSoft::triangleBoundingBox(glm::vec4 *vert, float width, float height) {
  float minX = std::min(std::min(vert[0].x, vert[1].x), vert[2].x);
  float minY = std::min(std::min(vert[0].y, vert[1].y), vert[2].y);
  float maxX = std::max(std::max(vert[0].x, vert[1].x), vert[2].x);
  float maxY = std::max(std::max(vert[0].y, vert[1].y), vert[2].y);

  minX = std::max(minX - 0.5f, 0.f);
  minY = std::max(minY - 0.5f, 0.f);
  maxX = std::min(maxX + 0.5f, width - 1.f);
  maxY = std::min(maxY + 0.5f, height - 1.f);

  auto min = glm::vec3(minX, minY, 0.f);
  auto max = glm::vec3(maxX, maxY, 0.f);
  return {min, max};
}

bool RendererSoft::barycentric(glm::aligned_vec4 *vert, glm::aligned_vec4 &v0, glm::aligned_vec4 &p,
                               glm::aligned_vec4 &bc) {
#ifdef SOFTGL_SIMD_OPT
  // Ref: https://geometrian.com/programming/tutorials/cross-product/index.php
  __m128 vec0 = _mm_sub_ps(_mm_load_ps(&vert[0].x), _mm_set_ps(0, p.x, v0.x, v0.x));
  __m128 vec1 = _mm_sub_ps(_mm_load_ps(&vert[1].x), _mm_set_ps(0, p.y, v0.y, v0.y));

  __m128 tmp0 = _mm_shuffle_ps(vec0, vec0, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 tmp1 = _mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(3, 1, 0, 2));
  __m128 tmp2 = _mm_mul_ps(tmp0, vec1);
  __m128 tmp3 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 u = _mm_sub_ps(_mm_mul_ps(tmp0, tmp1), tmp3);

  if (std::abs(MM_F32(u, 2)) < FLT_EPSILON) {
    return false;
  }

  u = _mm_div_ps(u, _mm_set1_ps(MM_F32(u, 2)));
  bc = {1.f - (MM_F32(u, 0) + MM_F32(u, 1)), MM_F32(u, 1), MM_F32(u, 0), 0.f};
#else
  glm::vec3 u = glm::cross(glm::vec3(vert[0]) - glm::vec3(v0.x, v0.x, p.x),
                           glm::vec3(vert[1]) - glm::vec3(v0.y, v0.y, p.y));
  if (std::abs(u.z) < FLT_EPSILON) {
    return false;
  }

  u /= u.z;
  bc = {1.f - (u.x + u.y), u.y, u.x, 0.f};
#endif

  if (bc.x < 0 || bc.y < 0 || bc.z < 0) {
    return false;
  }

  return true;
}

void RendererSoft::interpolateVertex(VertexHolder &out, VertexHolder &v0, VertexHolder &v1, float t) {
  out.vertexHolder = MemoryUtils::makeBuffer<uint8_t>(vao_->vertexStride);
  out.vertex = out.vertexHolder.get();
  out.varyingsHolder = MemoryUtils::makeAlignedBuffer<float>(varyingsAlignedCnt_);
  out.varyings = out.varyingsHolder.get();

  // interpolate vertex (only support float element right now)
  const float *vertexIn[2] = {(float *) v0.vertex, (float *) v1.vertex};
  interpolateLinear((float *) out.vertex, vertexIn, vao_->vertexStride / sizeof(float), t);

  // vertex shader
  vertexShaderImpl(out);
}

void RendererSoft::interpolateLinear(float *varsOut, const float *varsIn[2], size_t elemCnt, float t) {
  const float *inVar0 = varsIn[0];
  const float *inVar1 = varsIn[1];

  if (inVar0 == nullptr || inVar1 == nullptr) {
    return;
  }

  for (int i = 0; i < elemCnt; i++) {
    varsOut[i] = glm::mix(*(inVar0 + i), *(inVar1 + i), t);
  }
}

void RendererSoft::interpolateBarycentric(float *varsOut,
                                          const float *varsIn[3],
                                          size_t elemCnt,
                                          glm::aligned_vec4 &bc) {
  const float *inVar0 = varsIn[0];
  const float *inVar1 = varsIn[1];
  const float *inVar2 = varsIn[2];

  if (inVar0 == nullptr || inVar1 == nullptr || inVar2 == nullptr) {
    return;
  }

  bool simd_enabled = false;
#ifdef SOFTGL_SIMD_OPT
  if ((PTR_ADDR(inVar0) % SOFTGL_ALIGNMENT == 0) &&
      (PTR_ADDR(inVar1) % SOFTGL_ALIGNMENT == 0) &&
      (PTR_ADDR(inVar2) % SOFTGL_ALIGNMENT == 0) &&
      (PTR_ADDR(varsOut) % SOFTGL_ALIGNMENT == 0)) {
    simd_enabled = true;
  }
#endif

  if (simd_enabled) {
    interpolateBarycentricSIMD(varsOut, varsIn, elemCnt, bc);
  } else {
    for (int i = 0; i < elemCnt; i++) {
      varsOut[i] = glm::dot(bc, glm::vec4(*(inVar0 + i), *(inVar1 + i), *(inVar2 + i), 0.f));
    }
  }
}

void RendererSoft::interpolateBarycentricSIMD(float *varsOut,
                                              const float *varsIn[3],
                                              size_t elemCnt,
                                              glm::aligned_vec4 &bc) {
#ifdef SOFTGL_SIMD_OPT
  const float *inVar0 = varsIn[0];
  const float *inVar1 = varsIn[1];
  const float *inVar2 = varsIn[2];

  uint32_t idx = 0;
  uint32_t end;

  end = elemCnt & (~7);
  if (end > 0) {
    __m256 bc0 = _mm256_set1_ps(bc[0]);
    __m256 bc1 = _mm256_set1_ps(bc[1]);
    __m256 bc2 = _mm256_set1_ps(bc[2]);

    for (; idx < end; idx += 8) {
      __m256 sum = _mm256_mul_ps(_mm256_load_ps(inVar0 + idx), bc0);
      sum = _mm256_fmadd_ps(_mm256_load_ps(inVar1 + idx), bc1, sum);
      sum = _mm256_fmadd_ps(_mm256_load_ps(inVar2 + idx), bc2, sum);
      _mm256_store_ps(varsOut + idx, sum);
    }
  }

  end = (elemCnt - idx) & (~3);
  if (end > 0) {
    __m128 bc0 = _mm_set1_ps(bc[0]);
    __m128 bc1 = _mm_set1_ps(bc[1]);
    __m128 bc2 = _mm_set1_ps(bc[2]);

    for (; idx < end; idx += 4) {
      __m128 sum = _mm_mul_ps(_mm_load_ps(inVar0 + idx), bc0);
      sum = _mm_fmadd_ps(_mm_load_ps(inVar1 + idx), bc1, sum);
      sum = _mm_fmadd_ps(_mm_load_ps(inVar2 + idx), bc2, sum);
      _mm_store_ps(varsOut + idx, sum);
    }
  }

  for (; idx < elemCnt; idx++) {
    varsOut[idx] = 0;
    varsOut[idx] += *(inVar0 + idx) * bc[0];
    varsOut[idx] += *(inVar1 + idx) * bc[1];
    varsOut[idx] += *(inVar2 + idx) * bc[2];
  }
#endif
}

}
