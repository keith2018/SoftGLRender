/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "renderer_soft.h"
#include "base/simd.h"
#include "base/hash_utils.h"
#include "render/geometry.h"
#include "framebuffer_soft.h"
#include "texture_soft.h"
#include "uniform_soft.h"
#include "shader_program_soft.h"
#include "vertex_soft.h"
#include "blend_soft.h"
#include "depth_soft.h"

namespace SoftGL {

#define RASTER_MULTI_THREAD

// framebuffer
std::shared_ptr<FrameBuffer> RendererSoft::CreateFrameBuffer() {
  return std::make_shared<FrameBufferSoft>();
}

// texture
std::shared_ptr<Texture2D> RendererSoft::CreateTexture2D(bool multi_sample) {
  return std::make_shared<Texture2DSoft>(multi_sample);
}

std::shared_ptr<TextureCube> RendererSoft::CreateTextureCube() {
  return std::make_shared<TextureCubeSoft>();
}

std::shared_ptr<TextureDepth> RendererSoft::CreateTextureDepth(bool multi_sample) {
  return std::make_shared<TextureDepthSoft>(multi_sample);
}

// vertex
std::shared_ptr<VertexArrayObject> RendererSoft::CreateVertexArrayObject(const VertexArray &vertex_array) {
  return std::make_shared<VertexArrayObjectSoft>(vertex_array);
}

// shader program
std::shared_ptr<ShaderProgram> RendererSoft::CreateShaderProgram() {
  return std::make_shared<ShaderProgramSoft>();
}

// uniform
std::shared_ptr<UniformBlock> RendererSoft::CreateUniformBlock(const std::string &name, int size) {
  return std::make_shared<UniformBlockSoft>(name, size);
}

std::shared_ptr<UniformSampler> RendererSoft::CreateUniformSampler(const std::string &name, TextureType type) {
  return std::make_shared<UniformSamplerSoft>(name, type);
}

// pipeline
void RendererSoft::SetFrameBuffer(FrameBuffer &frame_buffer) {
  fbo_ = dynamic_cast<FrameBufferSoft *>(&frame_buffer);
}

void RendererSoft::SetViewPort(int x, int y, int width, int height) {
  viewport_.x = (float) x;
  viewport_.y = (float) y;
  viewport_.width = (float) width;
  viewport_.height = (float) height;

  viewport_.depth_near = 0.f;
  viewport_.depth_far = 1.f;

  if (reverse_z) {
    std::swap(viewport_.depth_near, viewport_.depth_far);
  }

  viewport_.depth_min = std::min(viewport_.depth_near, viewport_.depth_far);
  viewport_.depth_max = std::max(viewport_.depth_near, viewport_.depth_far);

  viewport_.inner_o.x = viewport_.x + viewport_.width / 2.f;
  viewport_.inner_o.y = viewport_.y + viewport_.height / 2.f;
  viewport_.inner_o.z = viewport_.depth_near;
  viewport_.inner_o.w = 0.f;

  viewport_.inner_p.x = viewport_.width / 2.f;    // divide by 2 in advance
  viewport_.inner_p.y = viewport_.height / 2.f;   // divide by 2 in advance
  viewport_.inner_p.z = viewport_.depth_far - viewport_.depth_near;
  viewport_.inner_p.w = 1.f;
}

void RendererSoft::Clear(const ClearState &state) {
  if (!fbo_) {
    return;
  }

  fbo_color_ = fbo_->GetColorBuffer();
  fbo_depth_ = fbo_->GetDepthBuffer();

  if (state.color_flag && fbo_color_) {
    RGBA color = RGBA(state.clear_color.r * 255,
                      state.clear_color.g * 255,
                      state.clear_color.b * 255,
                      state.clear_color.a * 255);
    if (fbo_color_->multi_sample) {
      fbo_color_->buffer_ms4x->SetAll(glm::tvec4<RGBA>(color));
    } else {
      fbo_color_->buffer->SetAll(color);
    }
  }

  if (state.depth_flag && fbo_depth_) {
    float depth = viewport_.depth_far;
    if (fbo_color_->multi_sample) {
      fbo_depth_->buffer_ms4x->SetAll(glm::tvec4<float>(depth));
    } else {
      fbo_depth_->buffer->SetAll(depth);
    }
  }
}

void RendererSoft::SetRenderState(const RenderState &state) {
  render_state_ = &state;
}

void RendererSoft::SetVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) {
  vao_ = dynamic_cast<VertexArrayObjectSoft *>(vao.get());
}

void RendererSoft::SetShaderProgram(std::shared_ptr<ShaderProgram> &program) {
  shader_program_ = dynamic_cast<ShaderProgramSoft *>(program.get());
}

void RendererSoft::SetShaderUniforms(std::shared_ptr<ShaderUniforms> &uniforms) {
  if (!uniforms) {
    return;
  }
  if (shader_program_) {
    shader_program_->BindUniforms(*uniforms);
  }
}

void RendererSoft::Draw(PrimitiveType type) {
  if (!fbo_ || !vao_ || !shader_program_) {
    return;
  }

  fbo_color_ = fbo_->GetColorBuffer();
  fbo_depth_ = fbo_->GetDepthBuffer();
  primitive_type_ = type;

  ProcessVertexShader();
  ProcessPrimitiveAssembly();
  ProcessClipping();
  ProcessPerspectiveDivide();
  ProcessViewportTransform();
  ProcessFaceCulling();
  ProcessRasterization();

  if (fbo_color_ && fbo_color_->multi_sample) {
    MultiSampleResolve();
  }
}

void RendererSoft::ProcessVertexShader() {
  // init shader varyings
  varyings_cnt_ = shader_program_->GetShaderVaryingsSize() / sizeof(float);
  varyings_aligned_size_ = MemoryUtils::AlignedSize(varyings_cnt_ * sizeof(float));
  varyings_aligned_cnt_ = varyings_aligned_size_ / sizeof(float);

  varyings_ = MemoryUtils::MakeAlignedBuffer<float>(vao_->vertex_cnt * varyings_aligned_cnt_);
  float *varying_buffer = varyings_.get();

  uint8_t *vertex_ptr = vao_->vertexes.data();
  vertexes_.resize(vao_->vertex_cnt);
  for (int idx = 0; idx < vao_->vertex_cnt; idx++) {
    VertexHolder &holder = vertexes_[idx];
    holder.discard = false;
    holder.index = idx;
    holder.vertex = vertex_ptr;
    holder.varyings = (varyings_aligned_size_ > 0) ? (varying_buffer + idx * varyings_aligned_cnt_) : nullptr;
    VertexShaderImpl(holder);
    vertex_ptr += vao_->vertex_stride;
  }
}

void RendererSoft::ProcessPrimitiveAssembly() {
  switch (primitive_type_) {
    case Primitive_POINT:
      ProcessPointAssembly();
      break;
    case Primitive_LINE:
      ProcessLineAssembly();
      break;
    case Primitive_TRIANGLE:
      ProcessPolygonAssembly();
      break;
  }
}

void RendererSoft::ProcessClipping() {
  size_t primitive_cnt = primitives_.size();
  for (int i = 0; i < primitive_cnt; i++) {
    auto &primitive = primitives_[i];
    if (primitive.discard) {
      continue;
    }
    switch (primitive_type_) {
      case Primitive_POINT:
        ClippingPoint(primitive);
        break;
      case Primitive_LINE:
        ClippingLine(primitive);
        break;
      case Primitive_TRIANGLE:
        // skip clipping if draw triangles with point/line mode
        if (render_state_->polygon_mode != PolygonMode_FILL) {
          continue;
        }
        ClippingTriangle(primitive);
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
    switch (primitive_type_) {
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

void RendererSoft::ProcessPerspectiveDivide() {
  for (auto &vertex : vertexes_) {
    if (vertex.discard) {
      continue;
    }
    PerspectiveDivideImpl(vertex);
  }
}

void RendererSoft::ProcessViewportTransform() {
  for (auto &vertex : vertexes_) {
    if (vertex.discard) {
      continue;
    }
    ViewportTransformImpl(vertex);
  }
}

void RendererSoft::ProcessFaceCulling() {
  if (primitive_type_ != Primitive_TRIANGLE) {
    return;
  }

  for (auto &triangle : primitives_) {
    if (triangle.discard) {
      continue;
    }

    glm::vec4 &v0 = vertexes_[triangle.indices[0]].frag_pos;
    glm::vec4 &v1 = vertexes_[triangle.indices[1]].frag_pos;
    glm::vec4 &v2 = vertexes_[triangle.indices[2]].frag_pos;

    glm::vec3 n = glm::cross(glm::vec3(v1 - v0), glm::vec3(v2 - v0));
    float area = glm::dot(n, glm::vec3(0, 0, 1));
    triangle.front_facing = area > 0;

    if (render_state_->cull_face) {
      triangle.discard = !triangle.front_facing;  // discard back face
    }
  }
}

void RendererSoft::ProcessRasterization() {
  switch (primitive_type_) {
    case Primitive_POINT:
      for (auto &primitive : primitives_) {
        if (primitive.discard) {
          continue;
        }
        auto *vert0 = &vertexes_[primitive.indices[0]];
        RasterizationPoint(vert0, render_state_->point_size);
      }
      break;
    case Primitive_LINE:
      for (auto &primitive : primitives_) {
        if (primitive.discard) {
          continue;
        }
        auto *vert0 = &vertexes_[primitive.indices[0]];
        auto *vert1 = &vertexes_[primitive.indices[1]];
        RasterizationLine(vert0, vert1, render_state_->line_width);
      }
      break;
    case Primitive_TRIANGLE:
      thread_quad_ctx_.resize(thread_pool_.GetThreadCnt());
      for (auto &ctx : thread_quad_ctx_) {
        ctx.SetVaryingsSize(varyings_aligned_cnt_);
        ctx.shader_program = shader_program_->clone();
        ctx.shader_program->PrepareFragmentShader();

        // setup derivative
        DerivativeContext &df_ctx = ctx.shader_program->GetShaderBuiltin().df_ctx;
        df_ctx.p0 = ctx.pixels[0].varyings_frag;
        df_ctx.p1 = ctx.pixels[1].varyings_frag;
        df_ctx.p2 = ctx.pixels[2].varyings_frag;
        df_ctx.p3 = ctx.pixels[3].varyings_frag;
      }
      RasterizationPolygons(primitives_);
      thread_pool_.WaitTasksFinish();
      break;
  }
}

void RendererSoft::ProcessFragmentShader(glm::vec4 &screen_pos,
                                         bool front_facing,
                                         void *varyings,
                                         ShaderProgramSoft *shader) {
  auto &builtin = shader->GetShaderBuiltin();
  builtin.FragCoord = screen_pos;
  builtin.FrontFacing = front_facing;

  shader->BindFragmentShaderVaryings(varyings);
  shader->ExecFragmentShader();
}

void RendererSoft::ProcessPerSampleOperations(int x, int y, float depth, const glm::vec4 &color, int sample) {
  // depth test
  if (!ProcessDepthTest(x, y, depth, sample, false)) {
    return;
  }

  if (!fbo_color_) {
    return;
  }

  glm::vec4 color_clamp = glm::clamp(color, 0.f, 1.f);

  // color blending
  ProcessColorBlending(x, y, color_clamp, sample);

  // write final color to fbo
  SetFrameColor(x, y, color_clamp * 255.f, sample);
}

bool RendererSoft::ProcessDepthTest(int x, int y, float depth, int sample, bool skip_write) {
  if (!render_state_->depth_test || !fbo_depth_) {
    return true;
  }

  // depth clamping
  depth = glm::clamp(depth, viewport_.depth_min, viewport_.depth_max);

  // depth comparison
  float *z_ptr = GetFrameDepth(x, y, sample);
  if (z_ptr && DepthTest(depth, *z_ptr, render_state_->depth_func)) {
    // depth attachment writes
    if (!skip_write && render_state_->depth_mask) {
      *z_ptr = depth;
    }
    return true;
  }
  return false;
}

void RendererSoft::ProcessColorBlending(int x, int y, glm::vec4 &color, int sample) {
  if (render_state_->blend) {
    glm::vec4 &src_color = color;
    glm::vec4 dst_color = glm::vec4(0.f);
    auto *ptr = GetFrameColor(x, y, sample);
    if (ptr) {
      dst_color = glm::vec4(*ptr) / 255.f;
    }
    color = CalcBlendColor(src_color, dst_color, render_state_->blend_parameters);
  }
}

void RendererSoft::ProcessPointAssembly() {
  primitives_.resize(vao_->indices_cnt);
  for (int idx = 0; idx < primitives_.size(); idx++) {
    auto &point = primitives_[idx];
    point.indices[0] = vao_->indices[idx];
    point.discard = false;
  }
}

void RendererSoft::ProcessLineAssembly() {
  primitives_.resize(vao_->indices_cnt / 2);
  for (int idx = 0; idx < primitives_.size(); idx++) {
    auto &line = primitives_[idx];
    line.indices[0] = vao_->indices[idx * 2];
    line.indices[1] = vao_->indices[idx * 2 + 1];
    line.discard = false;
  }
}

void RendererSoft::ProcessPolygonAssembly() {
  primitives_.resize(vao_->indices_cnt / 3);
  for (int idx = 0; idx < primitives_.size(); idx++) {
    auto &triangle = primitives_[idx];
    triangle.indices[0] = vao_->indices[idx * 3];
    triangle.indices[1] = vao_->indices[idx * 3 + 1];
    triangle.indices[2] = vao_->indices[idx * 3 + 2];
    triangle.discard = false;
  }
}

void RendererSoft::ClippingPoint(PrimitiveHolder &point) {
  point.discard = (vertexes_[point.indices[0]].clip_mask != 0);
}

void RendererSoft::ClippingLine(PrimitiveHolder &line, bool post_vertex_process) {
  VertexHolder *v0 = &vertexes_[line.indices[0]];
  VertexHolder *v1 = &vertexes_[line.indices[1]];

  bool full_clip = false;
  float t0 = 0.f;
  float t1 = 1.f;

  int mask = v0->clip_mask | v1->clip_mask;
  if (mask != 0) {
    for (int i = 0; i < 6; i++) {
      if (mask & FrustumClipMaskArray[i]) {
        float d0 = glm::dot(FrustumClipPlane[i], v0->clip_pos);
        float d1 = glm::dot(FrustumClipPlane[i], v1->clip_pos);

        if (d0 < 0 && d1 < 0) {
          full_clip = true;
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

  if (full_clip) {
    line.discard = true;
    return;
  }

  if (v0->clip_mask) {
    auto &vert = ClippingNewVertex(vertexes_[line.indices[0]],
                                   vertexes_[line.indices[1]], t0,
                                   post_vertex_process);
    line.indices[0] = vert.index;
  }

  if (v1->clip_mask) {
    auto &vert = ClippingNewVertex(vertexes_[line.indices[0]],
                                   vertexes_[line.indices[1]], t1,
                                   post_vertex_process);
    line.indices[1] = vert.index;
  }
}

void RendererSoft::ClippingTriangle(PrimitiveHolder &triangle) {
  auto *v0 = &vertexes_[triangle.indices[0]];
  auto *v1 = &vertexes_[triangle.indices[1]];
  auto *v2 = &vertexes_[triangle.indices[2]];

  int mask = v0->clip_mask | v1->clip_mask | v2->clip_mask;
  if (mask == 0) {
    return;
  }

  bool full_clip = false;
  std::vector<size_t> indices_in;
  std::vector<size_t> indices_out;

  indices_in.push_back(v0->index);
  indices_in.push_back(v1->index);
  indices_in.push_back(v2->index);

  for (int plane_idx = 0; plane_idx < 6; plane_idx++) {
    if (mask & FrustumClipMaskArray[plane_idx]) {
      if (indices_in.size() < 3) {
        full_clip = true;
        break;
      }
      indices_out.clear();
      size_t idx_pre = indices_in[0];
      float d_pre = glm::dot(FrustumClipPlane[plane_idx], vertexes_[idx_pre].clip_pos);

      indices_in.push_back(idx_pre);
      for (int i = 1; i < indices_in.size(); i++) {
        size_t idx = indices_in[i];
        float d = glm::dot(FrustumClipPlane[plane_idx], vertexes_[idx].clip_pos);

        if (d_pre >= 0) {
          indices_out.push_back(idx_pre);
        }

        if (std::signbit(d_pre) != std::signbit(d)) {
          float t = d < 0 ? d_pre / (d_pre - d) : -d_pre / (d - d_pre);
          // create new vertex
          auto &vert = ClippingNewVertex(vertexes_[idx_pre], vertexes_[idx], t);
          indices_out.push_back(vert.index);
        }

        idx_pre = idx;
        d_pre = d;
      }

      std::swap(indices_in, indices_out);
    }
  }

  if (full_clip || indices_in.empty()) {
    triangle.discard = true;
    return;
  }

  triangle.indices[0] = indices_in[0];
  triangle.indices[1] = indices_in[1];
  triangle.indices[2] = indices_in[2];

  for (int i = 3; i < indices_in.size(); i++) {
    primitives_.emplace_back();
    PrimitiveHolder &ph = primitives_.back();
    ph.discard = false;
    ph.indices[0] = indices_in[0];
    ph.indices[1] = indices_in[i - 1];
    ph.indices[2] = indices_in[i];
    ph.front_facing = triangle.front_facing;
  }
}

void RendererSoft::RasterizationPolygons(std::vector<PrimitiveHolder> &primitives) {
  switch (render_state_->polygon_mode) {
    case PolygonMode_POINT:
      RasterizationPolygonsPoint(primitives);
      break;
    case PolygonMode_LINE:
      RasterizationPolygonsLine(primitives);
      break;
    case PolygonMode_FILL:
      RasterizationPolygonsTriangle(primitives);
      break;
  }
}

void RendererSoft::RasterizationPolygonsPoint(std::vector<PrimitiveHolder> &primitives) {
  for (auto &triangle : primitives) {
    if (triangle.discard) {
      continue;
    }
    for (size_t idx : triangle.indices) {
      PrimitiveHolder point;
      point.discard = false;
      point.front_facing = triangle.front_facing;
      point.indices[0] = idx;

      // clipping
      ClippingPoint(point);
      if (point.discard) {
        continue;
      }

      // rasterization
      RasterizationPoint(&vertexes_[point.indices[0]],
                         render_state_->point_size);
    }
  }
}

void RendererSoft::RasterizationPolygonsLine(std::vector<PrimitiveHolder> &primitives) {
  for (auto &triangle : primitives) {
    if (triangle.discard) {
      continue;
    }
    for (size_t i = 0; i < 3; i++) {
      PrimitiveHolder line;
      line.discard = false;
      line.front_facing = triangle.front_facing;
      line.indices[0] = triangle.indices[i];
      line.indices[1] = triangle.indices[(i + 1) % 3];

      // clipping
      ClippingLine(line, true);
      if (line.discard) {
        continue;
      }

      // rasterization
      RasterizationLine(&vertexes_[line.indices[0]],
                        &vertexes_[line.indices[1]],
                        render_state_->line_width);
    }
  }
}

void RendererSoft::RasterizationPolygonsTriangle(std::vector<PrimitiveHolder> &primitives) {
  for (auto &triangle : primitives) {
    if (triangle.discard) {
      continue;
    }
    RasterizationTriangle(&vertexes_[triangle.indices[0]],
                          &vertexes_[triangle.indices[1]],
                          &vertexes_[triangle.indices[2]],
                          triangle.front_facing);
  }
}

void RendererSoft::RasterizationPoint(VertexHolder *v, float point_size) {
  if (!fbo_color_) {
    return;
  }

  float left = v->frag_pos.x - point_size / 2.f + 0.5f;
  float right = left + point_size;
  float top = v->frag_pos.y - point_size / 2.f + 0.5f;
  float bottom = top + point_size;

  glm::vec4 &screen_pos = v->frag_pos;
  for (int x = (int) left; x < (int) right; x++) {
    for (int y = (int) top; y < (int) bottom; y++) {
      screen_pos.x = (float) x;
      screen_pos.y = (float) y;
      ProcessFragmentShader(screen_pos, true, v->varyings, shader_program_);
      auto &builtIn = shader_program_->GetShaderBuiltin();
      if (!builtIn.discard) {
        // TODO MSAA
        for (int idx = 0; idx < fbo_color_->sample_cnt; idx++) {
          ProcessPerSampleOperations(x, y, screen_pos.z, builtIn.FragColor, idx);
        }
      }
    }
  }
}

void RendererSoft::RasterizationLine(VertexHolder *v0, VertexHolder *v1, float line_width) {
  // TODO diamond-exit rule
  int x0 = (int) v0->frag_pos.x, y0 = (int) v0->frag_pos.y;
  int x1 = (int) v1->frag_pos.x, y1 = (int) v1->frag_pos.y;

  float z0 = v0->frag_pos.z;
  float z1 = v1->frag_pos.z;

  float w0 = v0->frag_pos.w;
  float w1 = v1->frag_pos.w;

  bool steep = false;
  if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
    std::swap(x0, y0);
    std::swap(x1, y1);
    steep = true;
  }

  const float *varyings_in[2] = {v0->varyings, v1->varyings};

  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
    std::swap(z0, z1);
    std::swap(w0, w1);
    std::swap(varyings_in[0], varyings_in[1]);
  }
  int dx = x1 - x0;
  int dy = y1 - y0;

  int error = 0;
  int dError = 2 * std::abs(dy);

  int y = y0;

  auto varyings = MemoryUtils::MakeBuffer<float>(varyings_cnt_);
  VertexHolder pt{};
  pt.varyings = varyings.get();

  float t = 0;
  for (int x = x0; x <= x1; x++) {
    t = (float) (x - x0) / (float) dx;
    pt.frag_pos = glm::vec4(x, y, glm::mix(z0, z1, t), glm::mix(w0, w1, t));
    if (steep) {
      std::swap(pt.frag_pos.x, pt.frag_pos.y);
    }
    InterpolateLinear(pt.varyings, varyings_in, varyings_cnt_, t);
    RasterizationPoint(&pt, line_width);

    error += dError;
    if (error > dx) {
      y += (y1 > y0 ? 1 : -1);
      error -= 2 * dx;
    }
  }
}

void RendererSoft::RasterizationTriangle(VertexHolder *v0, VertexHolder *v1, VertexHolder *v2, bool front_facing) {
  // TODO top-left rule
  VertexHolder *vert[3] = {v0, v1, v2};
  glm::aligned_vec4 screen_pos[3] = {vert[0]->frag_pos, vert[1]->frag_pos, vert[2]->frag_pos};
  BoundingBox bounds = TriangleBoundingBox(screen_pos, viewport_.width, viewport_.height);
  bounds.min -= 1.f;

  auto block_size = raster_block_size_;
  int block_cnt_x = (bounds.max.x - bounds.min.x + block_size - 1.f) / block_size;
  int block_cnt_y = (bounds.max.y - bounds.min.y + block_size - 1.f) / block_size;

  for (int block_y = 0; block_y < block_cnt_y; block_y++) {
    for (int block_x = 0; block_x < block_cnt_x; block_x++) {
#ifdef RASTER_MULTI_THREAD
      thread_pool_.PushTask([&, vert, bounds, block_size, block_x, block_y](int thread_id) {
        // init pixel quad
        auto pixel_quad = thread_quad_ctx_[thread_id];
#else
        auto pixel_quad = thread_quad_ctx_[0];
#endif
        pixel_quad.front_facing = front_facing;

        for (int i = 0; i < 3; i++) {
          pixel_quad.vert_pos[i] = vert[i]->frag_pos;
          pixel_quad.vert_z[i] = &vert[i]->frag_pos.z;
          pixel_quad.vert_w[i] = vert[i]->frag_pos.w;
          pixel_quad.vert_varyings[i] = vert[i]->varyings;
        }

        glm::aligned_vec4 *vert_pos = pixel_quad.vert_pos;
        pixel_quad.vert_pos_flat[0] = {vert_pos[2].x, vert_pos[1].x, vert_pos[0].x, 0.f};
        pixel_quad.vert_pos_flat[1] = {vert_pos[2].y, vert_pos[1].y, vert_pos[0].y, 0.f};
        pixel_quad.vert_pos_flat[2] = {vert_pos[0].z, vert_pos[1].z, vert_pos[2].z, 0.f};
        pixel_quad.vert_pos_flat[3] = {vert_pos[0].w, vert_pos[1].w, vert_pos[2].w, 0.f};

        // block rasterization
        int block_start_x = bounds.min.x + block_x * block_size;
        int block_start_y = bounds.min.y + block_y * block_size;
        for (int y = block_start_y + 1; y < block_start_y + block_size && y <= bounds.max.y; y += 2) {
          for (int x = block_start_x + 1; x < block_start_x + block_size && x <= bounds.max.x; x += 2) {
            pixel_quad.Init((float) x, (float) y, fbo_color_->sample_cnt);
            RasterizationPixelQuad(pixel_quad);
          }
        }
#ifdef RASTER_MULTI_THREAD
      });
#endif
    }
  }
}

void RendererSoft::RasterizationPixelQuad(PixelQuadContext &quad) {
  glm::aligned_vec4 *vert = quad.vert_pos_flat;
  glm::aligned_vec4 &v0 = quad.vert_pos[0];

  // barycentric
  for (auto &pixel : quad.pixels) {
    for (auto &sample : pixel.samples) {
      sample.inside = Barycentric(vert, v0, sample.position, sample.barycentric);
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
      InterpolateBarycentric(&sample.position.z, quad.vert_z, 2, sample.barycentric);

      // depth clip
      if (sample.position.z < viewport_.depth_min || sample.position.z > viewport_.depth_max) {
        sample.inside = false;
      }

      // barycentric correction
      sample.barycentric *= (1.f / sample.position.w * quad.vert_w);
    }
  }

  // early z
  if (early_z && render_state_->depth_test) {
    if (!EarlyZTest(quad)) {
      return;
    }
  }

  // varying interpolate
  // note: all quad pixels should perform varying interpolate to enable varying partial derivative
  for (auto &pixel : quad.pixels) {
    InterpolateBarycentric((float *) pixel.varyings_frag,
                           quad.vert_varyings,
                           varyings_cnt_,
                           pixel.sample_shading->barycentric);
  }

  // pixel shading
  for (auto &pixel : quad.pixels) {
    if (!pixel.inside) {
      continue;
    }

    // fragment shader
    ProcessFragmentShader(pixel.sample_shading->position,
                          quad.front_facing,
                          pixel.varyings_frag,
                          quad.shader_program.get());

    // sample coverage
    auto &builtIn = quad.shader_program->GetShaderBuiltin();

    // per-sample operations
    if (pixel.sample_count > 1) {
      for (int idx = 0; idx < pixel.sample_count; idx++) {
        auto &sample = pixel.samples[idx];
        if (!sample.inside) {
          continue;
        }
        ProcessPerSampleOperations(sample.fbo_coord.x, sample.fbo_coord.y, sample.position.z, builtIn.FragColor, idx);
      }
    } else {
      auto &sample = *pixel.sample_shading;
      ProcessPerSampleOperations(sample.fbo_coord.x, sample.fbo_coord.y, sample.position.z, builtIn.FragColor, 0);
    }
  }
}

bool RendererSoft::EarlyZTest(PixelQuadContext &quad) {
  for (auto &pixel : quad.pixels) {
    if (!pixel.inside) {
      continue;
    }
    if (pixel.sample_count > 1) {
      bool inside = false;
      for (int idx = 0; idx < pixel.sample_count; idx++) {
        auto &sample = pixel.samples[idx];
        if (!sample.inside) {
          continue;
        }
        sample.inside = ProcessDepthTest(sample.fbo_coord.x, sample.fbo_coord.y, sample.position.z, idx, true);
        if (sample.inside) {
          inside = true;
        }
      }
      pixel.inside = inside;
    } else {
      auto &sample = *pixel.sample_shading;
      sample.inside = ProcessDepthTest(sample.fbo_coord.x, sample.fbo_coord.y, sample.position.z, 0, true);
      pixel.inside = sample.inside;
    }
  }
  return quad.CheckInside();
}

void RendererSoft::MultiSampleResolve() {
  if (!fbo_color_->buffer) {
    fbo_color_->buffer = Buffer<RGBA>::MakeDefault(fbo_color_->width, fbo_color_->height);
  }

  auto *src_ptr = fbo_color_->buffer_ms4x->GetRawDataPtr();
  auto *dst_ptr = fbo_color_->buffer->GetRawDataPtr();

  for (size_t row = 0; row < fbo_color_->height; row++) {
    auto *row_src = src_ptr + row * fbo_color_->width;
    auto *row_dst = dst_ptr + row * fbo_color_->width;
#ifdef RASTER_MULTI_THREAD
    thread_pool_.PushTask([&, row_src, row_dst](int thread_id) {
#endif
      auto *src = row_src;
      auto *dst = row_dst;
      for (size_t idx = 0; idx < fbo_color_->width; idx++) {
        glm::vec4 color(0.f);
        for (int i = 0; i < fbo_color_->sample_cnt; i++) {
          color += (glm::vec4) (*src)[i];
        }
        color /= fbo_color_->sample_cnt;
        *dst = color;
        src++;
        dst++;
      }
#ifdef RASTER_MULTI_THREAD
    });
#endif
  }

  thread_pool_.WaitTasksFinish();
}

RGBA *RendererSoft::GetFrameColor(int x, int y, int sample) {
  if (!fbo_color_) {
    return nullptr;
  }

  RGBA *ptr = nullptr;
  if (fbo_color_->multi_sample) {
    auto *ptr_ms = fbo_color_->buffer_ms4x->Get(x, y);
    if (ptr_ms) {
      ptr = (RGBA *) ptr_ms + sample;
    }
  } else {
    ptr = fbo_color_->buffer->Get(x, y);
  }

  return ptr;
}

float *RendererSoft::GetFrameDepth(int x, int y, int sample) {
  if (!fbo_depth_) {
    return nullptr;
  }

  float *depth_ptr = nullptr;
  if (fbo_depth_->multi_sample) {
    auto *ptr = fbo_depth_->buffer_ms4x->Get(x, y);
    if (ptr) {
      depth_ptr = &ptr->x + sample;
    }
  } else {
    depth_ptr = fbo_depth_->buffer->Get(x, y);
  }
  return depth_ptr;
}

void RendererSoft::SetFrameColor(int x, int y, const RGBA &color, int sample) {
  RGBA *ptr = GetFrameColor(x, y, sample);
  if (ptr) {
    *ptr = color;
  }
}

VertexHolder &RendererSoft::ClippingNewVertex(VertexHolder &v0, VertexHolder &v1, float t, bool post_vertex_process) {
  vertexes_.emplace_back();
  VertexHolder &vh = vertexes_.back();
  vh.discard = false;
  vh.index = vertexes_.size() - 1;
  InterpolateVertex(vh, v0, v1, t);

  if (post_vertex_process) {
    PerspectiveDivideImpl(vh);
    ViewportTransformImpl(vh);
  }

  return vh;
}

void RendererSoft::VertexShaderImpl(VertexHolder &vertex) {
  shader_program_->BindVertexAttributes(vertex.vertex);
  shader_program_->BindVertexShaderVaryings(vertex.varyings);
  shader_program_->ExecVertexShader();

  vertex.clip_pos = shader_program_->GetShaderBuiltin().Position;
  vertex.clip_mask = CountFrustumClipMask(vertex.clip_pos);
}

void RendererSoft::PerspectiveDivideImpl(VertexHolder &vertex) {
  vertex.frag_pos = vertex.clip_pos;
  auto &pos = vertex.frag_pos;
  float inv_w = 1.f / pos.w;
  pos *= inv_w;
  pos.w = inv_w;
}

void RendererSoft::ViewportTransformImpl(VertexHolder &vertex) {
  vertex.frag_pos *= viewport_.inner_p;
  vertex.frag_pos += viewport_.inner_o;
}

int RendererSoft::CountFrustumClipMask(glm::vec4 &clip_pos) {
  int mask = 0;
  if (clip_pos.w < clip_pos.x) mask |= FrustumClipMask::POSITIVE_X;
  if (clip_pos.w < -clip_pos.x) mask |= FrustumClipMask::NEGATIVE_X;
  if (clip_pos.w < clip_pos.y) mask |= FrustumClipMask::POSITIVE_Y;
  if (clip_pos.w < -clip_pos.y) mask |= FrustumClipMask::NEGATIVE_Y;
  if (clip_pos.w < clip_pos.z) mask |= FrustumClipMask::POSITIVE_Z;
  if (clip_pos.w < -clip_pos.z) mask |= FrustumClipMask::NEGATIVE_Z;
  return mask;
}

BoundingBox RendererSoft::TriangleBoundingBox(glm::vec4 *vert, float width, float height) {
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

bool RendererSoft::Barycentric(glm::aligned_vec4 *vert,
                               glm::aligned_vec4 &v0,
                               glm::aligned_vec4 &p,
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

void RendererSoft::InterpolateVertex(VertexHolder &out, VertexHolder &v0, VertexHolder &v1, float t) {
  out.vertex_holder = MemoryUtils::MakeBuffer<uint8_t>(vao_->vertex_stride);
  out.vertex = out.vertex_holder.get();
  out.varyings_holder = MemoryUtils::MakeAlignedBuffer<float>(varyings_aligned_cnt_);
  out.varyings = out.varyings_holder.get();

  // interpolate vertex (only support float element right now)
  const float *vertex_in[2] = {(float *) v0.vertex, (float *) v1.vertex};
  InterpolateLinear((float *) out.vertex, vertex_in, vao_->vertex_stride / sizeof(float), t);

  // vertex shader
  VertexShaderImpl(out);
}

void RendererSoft::InterpolateLinear(float *vars_out,
                                     const float *vars_in[2],
                                     size_t elem_cnt,
                                     float t) {
  const float *in_var0 = vars_in[0];
  const float *in_var1 = vars_in[1];

  if (in_var0 == nullptr || in_var1 == nullptr) {
    return;
  }

  for (int i = 0; i < elem_cnt; i++) {
    vars_out[i] = glm::mix(*(in_var0 + i), *(in_var1 + i), t);
  }
}

void RendererSoft::InterpolateBarycentric(float *vars_out,
                                          const float *vars_in[3],
                                          size_t elem_cnt,
                                          glm::aligned_vec4 &bc) {
  const float *in_var0 = vars_in[0];
  const float *in_var1 = vars_in[1];
  const float *in_var2 = vars_in[2];

  if (in_var0 == nullptr || in_var1 == nullptr || in_var2 == nullptr) {
    return;
  }

  bool simd_enabled = false;
#ifdef SOFTGL_SIMD_OPT
  if ((PTR_ADDR(in_var0) % SOFTGL_ALIGNMENT == 0) &&
      (PTR_ADDR(in_var1) % SOFTGL_ALIGNMENT == 0) &&
      (PTR_ADDR(in_var2) % SOFTGL_ALIGNMENT == 0) &&
      (PTR_ADDR(vars_out) % SOFTGL_ALIGNMENT == 0)) {
    simd_enabled = true;
  }
#endif

  if (simd_enabled) {
    InterpolateBarycentricSIMD(vars_out, vars_in, elem_cnt, bc);
  } else {
    for (int i = 0; i < elem_cnt; i++) {
      vars_out[i] = glm::dot(bc, glm::vec4(*(in_var0 + i), *(in_var1 + i), *(in_var2 + i), 0.f));
    }
  }
}

void RendererSoft::InterpolateBarycentricSIMD(float *vars_out,
                                              const float *vars_in[3],
                                              size_t elem_cnt,
                                              glm::aligned_vec4 &bc) {
#ifdef SOFTGL_SIMD_OPT
  const float *in_var0 = vars_in[0];
  const float *in_var1 = vars_in[1];
  const float *in_var2 = vars_in[2];

  uint32_t idx = 0;
  uint32_t end;

  end = elem_cnt & (~7);
  if (end > 0) {
    __m256 bc0 = _mm256_set1_ps(bc[0]);
    __m256 bc1 = _mm256_set1_ps(bc[1]);
    __m256 bc2 = _mm256_set1_ps(bc[2]);

    for (; idx < end; idx += 8) {
      __m256 sum = _mm256_mul_ps(_mm256_load_ps(in_var0 + idx), bc0);
      sum = _mm256_fmadd_ps(_mm256_load_ps(in_var1 + idx), bc1, sum);
      sum = _mm256_fmadd_ps(_mm256_load_ps(in_var2 + idx), bc2, sum);
      _mm256_store_ps(vars_out + idx, sum);
    }
  }

  end = (elem_cnt - idx) & (~3);
  if (end > 0) {
    __m128 bc0 = _mm_set1_ps(bc[0]);
    __m128 bc1 = _mm_set1_ps(bc[1]);
    __m128 bc2 = _mm_set1_ps(bc[2]);

    for (; idx < end; idx += 4) {
      __m128 sum = _mm_mul_ps(_mm_load_ps(in_var0 + idx), bc0);
      sum = _mm_fmadd_ps(_mm_load_ps(in_var1 + idx), bc1, sum);
      sum = _mm_fmadd_ps(_mm_load_ps(in_var2 + idx), bc2, sum);
      _mm_store_ps(vars_out + idx, sum);
    }
  }

  for (; idx < elem_cnt; idx++) {
    vars_out[idx] = 0;
    vars_out[idx] += *(in_var0 + idx) * bc[0];
    vars_out[idx] += *(in_var1 + idx) * bc[1];
    vars_out[idx] += *(in_var2 + idx) * bc[2];
  }
#endif
}

}
