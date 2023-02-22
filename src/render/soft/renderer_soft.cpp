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

// framebuffer
std::shared_ptr<FrameBuffer> RendererSoft::CreateFrameBuffer() {
  return std::make_shared<FrameBufferSoft>();
}

// texture
std::shared_ptr<Texture2D> RendererSoft::CreateTexture2D() {
  return std::make_shared<Texture2DSoft>();
}

std::shared_ptr<TextureCube> RendererSoft::CreateTextureCube() {
  return std::make_shared<TextureCubeSoft>();
}

std::shared_ptr<TextureDepth> RendererSoft::CreateTextureDepth() {
  return std::make_shared<TextureDepthSoft>();
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
    fbo_color_->SetAll(glm::u8vec4(state.clear_color.r * 255,
                                   state.clear_color.g * 255,
                                   state.clear_color.b * 255,
                                   state.clear_color.a * 255));

  }

  if (state.depth_flag && fbo_depth_) {
    fbo_depth_->SetAll(viewport_.depth_far);
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
  if (!fbo_) {
    return;
  }

  fbo_color_ = fbo_->GetColorBuffer();
  fbo_depth_ = fbo_->GetDepthBuffer();
  if (!fbo_color_ || !vao_ || !shader_program_) {
    return;
  }

  primitive_type_ = type;

  ProcessVertexShader();
  ProcessPrimitiveAssembly();
  ProcessClipping();
  ProcessPerspectiveDivide();
  ProcessViewportTransform();
  ProcessFaceCulling();
  ProcessRasterization();
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

    glm::vec4 &v0 = vertexes_[triangle.indices[0]].src_pos;
    glm::vec4 &v1 = vertexes_[triangle.indices[1]].src_pos;
    glm::vec4 &v2 = vertexes_[triangle.indices[2]].src_pos;

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
  auto pos_x = (int) screen_pos.x;
  auto pos_y = (int) screen_pos.y;

  auto &builtin = shader->GetShaderBuiltin();
  builtin.FragCoord = screen_pos;
  builtin.FragCoord.w = 1.f / builtin.FragCoord.w;
  builtin.FrontFacing = front_facing;
  builtin.FragDepth = builtin.FragCoord.z;   // default depth

  shader->BindFragmentShaderVaryings(varyings);
  shader->ExecFragmentShader();
  if (builtin.discard) {
    return;
  }

  ProcessPerSampleOperations(pos_x, pos_y, builtin);
}

void RendererSoft::ProcessPerSampleOperations(int x, int y, ShaderBuiltin &builtin) {
  // depth test
  if (!ProcessDepthTest(x, y, builtin.FragDepth)) {
    return;
  }

  glm::vec4 color = glm::clamp(builtin.FragColor, 0.f, 1.f);

  // color blending
  ProcessColorBlending(x, y, color);

  // write final color to fbo
  SetFrameColor(x, y, color * 255.f);
}

bool RendererSoft::ProcessDepthTest(int x, int y, float depth) {
  if (!render_state_->depth_test) {
    return true;
  }

  // depth clamping
  depth = glm::clamp(depth, viewport_.depth_min, viewport_.depth_max);

  // depth comparison
  float *z_ptr = fbo_depth_->Get(x, y);
  if (z_ptr && DepthTest(depth, *z_ptr, render_state_->depth_func)) {
    // depth attachment writes
    if (render_state_->depth_mask) {
      *z_ptr = depth;
    }
    return true;
  }
  return false;
}

void RendererSoft::ProcessColorBlending(int x, int y, glm::vec4 &color) {
  if (render_state_->blend) {
    glm::vec4 &src_color = color;
    glm::vec4 dst_color = glm::vec4(GetFrameColor(x, y)) / 255.f;
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
  float left = v->src_pos.x - point_size / 2.f + 0.5f;
  float right = left + point_size;
  float top = v->src_pos.y - point_size / 2.f + 0.5f;
  float bottom = top + point_size;

  glm::vec4 &screen_pos = v->src_pos;
  for (int x = (int) left; x < (int) right; x++) {
    for (int y = (int) top; y < (int) bottom; y++) {
      screen_pos.x = (float) x;
      screen_pos.y = (float) y;
      ProcessFragmentShader(screen_pos, true, v->varyings, shader_program_);
    }
  }
}

void RendererSoft::RasterizationLine(VertexHolder *v0, VertexHolder *v1, float line_width) {
  int x0 = (int) v0->src_pos.x, y0 = (int) v0->src_pos.y;
  int x1 = (int) v1->src_pos.x, y1 = (int) v1->src_pos.y;

  float z0 = v0->src_pos.z;
  float z1 = v1->src_pos.z;

  float w0 = v0->src_pos.w;
  float w1 = v1->src_pos.w;

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

  float z = z0;
  float w = w0;
  float dz = (x1 == x0) ? 0 : (z1 - z0) / (float) (x1 - x0);
  float dw = (x1 == x0) ? 0 : (w1 - w0) / (float) (x1 - x0);

  auto varyings = MemoryUtils::MakeBuffer<float>(varyings_cnt_);
  VertexHolder pt{};
  pt.varyings = varyings.get();

  float t = 0;
  for (int x = x0; x <= x1; x++) {
    t = (float) (x - x0) / (float) dx;
    z += dz;
    w += dw;

    pt.src_pos = glm::vec4(x, y, z, w);
    if (steep) {
      std::swap(pt.src_pos.x, pt.src_pos.y);
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
  VertexHolder *vert[3] = {v0, v1, v2};
  glm::aligned_vec4 screen_pos[3] = {vert[0]->src_pos, vert[1]->src_pos, vert[2]->src_pos};
  BoundingBox bounds = TriangleBoundingBox(screen_pos, viewport_.width, viewport_.height);
  bounds.min -= 1.f;

  auto block_size = raster_block_size_;
  int block_cnt_x = (bounds.max.x - bounds.min.x + block_size - 1.f) / block_size;
  int block_cnt_y = (bounds.max.y - bounds.min.y + block_size - 1.f) / block_size;

  for (int block_y = 0; block_y < block_cnt_y; block_y++) {
    for (int block_x = 0; block_x < block_cnt_x; block_x++) {
      thread_pool_.PushTask([&, vert, bounds, block_size, block_x, block_y](int thread_id) {
        // init pixel quad
        auto pixel_quad = thread_quad_ctx_[thread_id];
        pixel_quad.front_facing = front_facing;
        glm::aligned_vec4 *vert_pos = pixel_quad.vert_pos;

        for (int i = 0; i < 3; i++) {
          pixel_quad.vert_pos[i] = vert[i]->src_pos;
          pixel_quad.vert_bc_factor[i] = vert[i]->clip_pos.z + vert[i]->clip_pos.w;
          pixel_quad.vert_varyings[i] = vert[i]->varyings;
        }

        pixel_quad.vert_pos_flat[0] = {vert_pos[2].x, vert_pos[1].x, vert_pos[0].x, 0.f};
        pixel_quad.vert_pos_flat[1] = {vert_pos[2].y, vert_pos[1].y, vert_pos[0].y, 0.f};
        pixel_quad.vert_pos_flat[2] = {vert_pos[0].z, vert_pos[1].z, vert_pos[2].z, 0.f};
        pixel_quad.vert_pos_flat[3] = {vert_pos[0].w, vert_pos[1].w, vert_pos[2].w, 0.f};

        // block rasterization
        int block_start_x = bounds.min.x + block_x * block_size;
        int block_start_y = bounds.min.y + block_y * block_size;
        for (int y = block_start_y + 1; y < block_start_y + block_size && y <= bounds.max.y; y += 2) {
          for (int x = block_start_x + 1; x < block_start_x + block_size && x <= bounds.max.x; x += 2) {
            pixel_quad.Init((float) x, (float) y);
            RasterizationPixelQuad(pixel_quad);
          }
        }
      });
    }
  }
}

void RendererSoft::RasterizationPixelQuad(PixelQuadContext &quad) {
  glm::aligned_vec4 *vert = quad.vert_pos_flat;
  glm::aligned_vec4 &v0 = quad.vert_pos[0];

  // barycentric
  for (auto &pixel : quad.pixels) {
    pixel.inside = Barycentric(vert, v0, pixel.position, pixel.barycentric);
  }
  if (!quad.QuadInside()) {
    return;
  }

  // barycentric correction
  BarycentricCorrect(quad);

  // varying interpolate
  // note: all quad pixels should perform varying interpolate to enable varying partial derivative
  for (auto &pixel : quad.pixels) {
    InterpolateBarycentric((float *) pixel.varyings_frag, quad.vert_varyings, varyings_cnt_, pixel.barycentric);
  }

  // pixel shading
  for (auto &pixel : quad.pixels) {
    if (!pixel.inside) {
      continue;
    }

    // fragment shader
    ProcessFragmentShader(pixel.position, quad.front_facing, pixel.varyings_frag, quad.shader_program.get());
  }
}

glm::u8vec4 RendererSoft::GetFrameColor(int x, int y) {
  return *fbo_color_->Get(x, y);
}

void RendererSoft::SetFrameColor(int x, int y, const glm::u8vec4 &color) {
  fbo_color_->Set(x, y, color);
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
  vertex.src_pos = vertex.clip_pos;
  auto &pos = vertex.src_pos;
  float inv_w = 1.0f / pos.w;
  pos.w *= pos.w;
  pos *= inv_w;
}

void RendererSoft::ViewportTransformImpl(VertexHolder &vertex) {
  vertex.src_pos *= viewport_.inner_p;
  vertex.src_pos += viewport_.inner_o;
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
  __m128 vec0 = _mm_sub_ps(_mm_load_ps(&vert[0].x), _mm_set_ps(0, p.x + 0.5f, v0.x, v0.x));
  __m128 vec1 = _mm_sub_ps(_mm_load_ps(&vert[1].x), _mm_set_ps(0, p.y + 0.5f, v0.y, v0.y));

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
  glm::vec3 u = glm::cross(glm::vec3(vert[0]) - glm::vec3(v0.x, v0.x, p.x + 0.5f),
                           glm::vec3(vert[1]) - glm::vec3(v0.y, v0.y, p.y + 0.5f));
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

void RendererSoft::BarycentricCorrect(PixelQuadContext &quad) {
  glm::aligned_vec4 *vert = quad.vert_pos_flat;
#ifdef SOFTGL_SIMD_OPT
  __m128 m_bc_factor = _mm_load_ps(&quad.vert_bc_factor.x);
  __m128 m_screen_z = _mm_load_ps(&vert[2].x);
  __m128 m_screen_w = _mm_load_ps(&vert[3].x);
  for (auto &pixel : quad.pixels) {
    auto &bc = pixel.barycentric;
    __m128 m_bc = _mm_load_ps(&bc.x);

    // barycentric correction
    m_bc = _mm_div_ps(m_bc, m_bc_factor);
    m_bc = _mm_div_ps(m_bc, _mm_set1_ps(MM_F32(m_bc, 0) + MM_F32(m_bc, 1) + MM_F32(m_bc, 2)));
    _mm_store_ps(&bc.x, m_bc);

    // interpolate z, w
    auto dz = _mm_dp_ps(m_screen_z, m_bc, 0x7f);
    auto dw = _mm_dp_ps(m_screen_w, m_bc, 0x7f);

    pixel.position.z = MM_F32(dz, 0);
    pixel.position.w = MM_F32(dw, 0);
  }
#else
  for (auto &pixel : quad.pixels) {
    auto &bc = pixel.barycentric;

    // barycentric correction
    bc /= quad.vert_bc_factor;
    bc /= (bc.x + bc.y + bc.z);

    // interpolate z, w
    pixel.position.z = glm::dot(vert[2], bc);
    pixel.position.w = glm::dot(vert[3], bc);
  }
#endif
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

void RendererSoft::InterpolateLinear(float *varyings_out,
                                     const float *varyings_in[2],
                                     size_t elem_cnt,
                                     float t) {
  const float *in_vary0 = varyings_in[0];
  const float *in_vary1 = varyings_in[1];

  if (in_vary0 == nullptr || in_vary1 == nullptr) {
    return;
  }

  for (int i = 0; i < elem_cnt; i++) {
    varyings_out[i] = glm::mix(*(in_vary0 + i), *(in_vary1 + i), t);
  }
}

void RendererSoft::InterpolateBarycentric(float *varyings_out,
                                          const float *varyings_in[3],
                                          size_t elem_cnt,
                                          glm::aligned_vec4 &bc) {
  const float *in_vary0 = varyings_in[0];
  const float *in_vary1 = varyings_in[1];
  const float *in_vary2 = varyings_in[2];

  if (in_vary0 == nullptr || in_vary1 == nullptr || in_vary2 == nullptr) {
    return;
  }

#ifdef SOFTGL_SIMD_OPT
  assert(PTR_ADDR(in_vary0) % SOFTGL_ALIGNMENT == 0);
  assert(PTR_ADDR(in_vary1) % SOFTGL_ALIGNMENT == 0);
  assert(PTR_ADDR(in_vary2) % SOFTGL_ALIGNMENT == 0);
  assert(PTR_ADDR(varyings_out) % SOFTGL_ALIGNMENT == 0);

  uint32_t idx = 0;
  uint32_t end;

  end = elem_cnt & (~7);
  if (end > 0) {
    __m256 bc0 = _mm256_set1_ps(bc[0]);
    __m256 bc1 = _mm256_set1_ps(bc[1]);
    __m256 bc2 = _mm256_set1_ps(bc[2]);

    for (; idx < end; idx += 8) {
      __m256 sum = _mm256_mul_ps(_mm256_load_ps(in_vary0 + idx), bc0);
      sum = _mm256_fmadd_ps(_mm256_load_ps(in_vary1 + idx), bc1, sum);
      sum = _mm256_fmadd_ps(_mm256_load_ps(in_vary2 + idx), bc2, sum);
      _mm256_store_ps(varyings_out + idx, sum);
    }
  }

  end = (elem_cnt - idx) & (~3);
  if (end > 0) {
    __m128 bc0 = _mm_set1_ps(bc[0]);
    __m128 bc1 = _mm_set1_ps(bc[1]);
    __m128 bc2 = _mm_set1_ps(bc[2]);

    for (; idx < end; idx += 4) {
      __m128 sum = _mm_mul_ps(_mm_load_ps(in_vary0 + idx), bc0);
      sum = _mm_fmadd_ps(_mm_load_ps(in_vary1 + idx), bc1, sum);
      sum = _mm_fmadd_ps(_mm_load_ps(in_vary2 + idx), bc2, sum);
      _mm_store_ps(varyings_out + idx, sum);
    }
  }

  for (; idx < elem_cnt; idx++) {
    varyings_out[idx] = 0;
    varyings_out[idx] += *(in_vary0 + idx) * bc[0];
    varyings_out[idx] += *(in_vary1 + idx) * bc[1];
    varyings_out[idx] += *(in_vary2 + idx) * bc[2];
  }
#else
  for (int i = 0; i < elem_cnt; i++) {
    varyings_out[i] = glm::dot(bc, glm::vec4(*(in_vary0 + i), *(in_vary1 + i), *(in_vary2 + i), 0.f));
  }
#endif
}

}
