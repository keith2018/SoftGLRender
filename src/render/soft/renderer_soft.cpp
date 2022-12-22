/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "renderer_soft.h"
#include "graphic.h"

namespace SoftGL {

void RendererSoft::Create(int w, int h, float near, float far) {
  Renderer::Create(w, h, near, far);

  depth_func = Depth_GREATER;  // Reversed-Z
  depth_test = true;
  depth_mask = true;
  cull_face_back = true;

  fbo_.width = width_;
  fbo_.height = height_;
  fbo_.Create(fbo_.width, fbo_.height);

  viewport_ = Viewport(0, 0, (float) width_, (float) height_);
  depth_range.Set(near, far);

  graphic_.DrawPoint = std::bind(&RendererSoft::DrawFramePointWithDepth, this, std::placeholders::_1,
                                 std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
}

void RendererSoft::Clear(float r, float g, float b, float a) {
  fbo_.color->SetAll(glm::u8vec4(r * 255, g * 255, b * 255, a * 255));
  fbo_.depth->SetAll(depth_range.near);
}

void RendererSoft::DrawMeshTextured(ModelMesh &mesh) {
  render_ctx_.Prepare(&mesh, shader_context_.varyings_size / sizeof(float));
  ProcessVertexShader();
  ProcessFaceFrustumClip();
  ProcessFaceScreenMapping();
  ProcessFaceFrontBackCull();
  ProcessFaceRasterization();
}

void RendererSoft::DrawMeshWireframe(ModelMesh &mesh) {
  render_ctx_.Prepare(&mesh, shader_context_.varyings_size / sizeof(float));
  ProcessVertexShader();
  if (wireframe_show_clip) {
    ProcessFaceFrustumClip();
    ProcessFaceScreenMapping();
    ProcessFaceWireframe(false);
  } else {
    ProcessFaceWireframe(true);
  }
}

void RendererSoft::DrawLines(ModelLines &lines) {
  render_ctx_.Prepare(&lines);
  ProcessVertexShader();

  glm::u8vec4 color = lines.line_color * 255.f;
  // line_width not support
  for (int line_idx = 0; line_idx < lines.primitive_cnt; line_idx++) {
    int idx0 = lines.indices[line_idx * 2];
    int idx1 = lines.indices[line_idx * 2 + 1];

    auto v0 = render_ctx_.vertexes[idx0];
    auto v1 = render_ctx_.vertexes[idx1];

    DrawLineFrustumClip(v0.pos, v0.clip_mask, v1.pos, v1.clip_mask, color);
  }
}

void RendererSoft::DrawPoints(ModelPoints &points) {
  render_ctx_.Prepare(&points);
  ProcessVertexShader();

  for (int pt_idx = 0; pt_idx < points.primitive_cnt; pt_idx++) {
    auto pt = render_ctx_.vertexes[pt_idx];
    if (pt.clip_mask != 0) {
      continue;
    }

    PerspectiveDivide(pt.pos);
    ViewportTransform(pt.pos);

    glm::u8vec4 color = points.point_color * 255.f;
    int radius = (int) points.point_size;
    graphic_.DrawCircle((int) pt.pos.x, (int) pt.pos.y, radius, pt.pos.z, color);
  }
}

void RendererSoft::ProcessVertexShader() {
  for (auto &vh : render_ctx_.vertexes) {
    VertexShaderImpl(vh);
  }
}

void RendererSoft::VertexShaderImpl(VertexHolder &vh) {
  auto shader = shader_context_.vertex_shader;
  shader->attributes = (ShaderAttributes *) vh.vertex;

  if (vh.varyings == nullptr) {
    shader->varyings = nullptr;
  } else {
    shader->varyings = (BaseShaderVaryings *) vh.varyings;
  }
  shader->shader_main();

  vh.pos = shader->gl_Position;
  vh.clip_mask = CountFrustumClipMask(vh.pos);
}

void RendererSoft::ProcessFaceFrustumClip() {
  if (!frustum_clip) {
    return;
  }

  size_t face_cnt = render_ctx_.faces.size();
  for (int face_idx = 0; face_idx < face_cnt; face_idx++) {
    auto &fh = render_ctx_.faces[face_idx];
    int idx0 = fh.indices[0];
    int idx1 = fh.indices[1];
    int idx2 = fh.indices[2];

    int mask = render_ctx_.vertexes[idx0].clip_mask | render_ctx_.vertexes[idx1].clip_mask
        | render_ctx_.vertexes[idx2].clip_mask;
    if (mask == 0) {
      continue;
    }

    bool full_clip = false;
    std::vector<int> indices_in;
    std::vector<int> indices_out;

    indices_in.push_back(idx0);
    indices_in.push_back(idx1);
    indices_in.push_back(idx2);

    for (int plane_idx = 0; plane_idx < 6; plane_idx++) {
      if (mask & FrustumClipMaskArray[plane_idx]) {
        if (indices_in.size() < 3) {
          full_clip = true;
          break;
        }

        indices_out.clear();

        int idx_pre = indices_in[0];
        float d_pre = glm::dot(FrustumClipPlane[plane_idx], render_ctx_.vertexes[idx_pre].pos);

        indices_in.push_back(idx_pre);

        for (int i = 1; i < indices_in.size(); i++) {
          int idx = indices_in[i];
          float d = glm::dot(FrustumClipPlane[plane_idx], render_ctx_.vertexes[idx].pos);

          if (d_pre >= 0) {
            indices_out.push_back(idx_pre);
          }

          if (std::signbit(d_pre) != std::signbit(d)) {
            float t = d < 0 ? d_pre / (d_pre - d) : -d_pre / (d - d_pre);
            VertexHolder vh = render_ctx_.VertexHolderInterpolate(&render_ctx_.vertexes[idx_pre],
                                                                  &render_ctx_.vertexes[idx],
                                                                  t);
            VertexShaderImpl(vh);
            render_ctx_.vertexes.push_back(vh);
            indices_out.push_back((int) (render_ctx_.vertexes.size() - 1));
          }

          idx_pre = idx;
          d_pre = d;
        }

        std::swap(indices_in, indices_out);
      }
    }

    if (full_clip || indices_in.empty()) {
      fh.discard = true;
      continue;
    }

    fh.indices[0] = indices_in[0];
    fh.indices[1] = indices_in[1];
    fh.indices[2] = indices_in[2];

    for (int i = 3; i < indices_in.size(); i++) {
      FaceHolder new_fh{};
      new_fh.indices[0] = indices_in[0];
      new_fh.indices[1] = indices_in[i - 1];
      new_fh.indices[2] = indices_in[i];
      new_fh.discard = false;
      render_ctx_.faces.push_back(new_fh);
    }
  }
}

void RendererSoft::ProcessFaceScreenMapping() {
  std::vector<bool> indices;
  indices.resize(render_ctx_.vertexes.size(), false);

  for (auto &fh : render_ctx_.faces) {
    if (fh.discard) {
      continue;
    }

    for (int i = 0; i < 3; i++) {
      indices[fh.indices[i]] = true;
    }
  }

  for (int i = 0; i < indices.size(); i++) {
    auto &vh = render_ctx_.vertexes[i];
    PerspectiveDivide(vh.pos);
    ViewportTransform(vh.pos);
  }
}

void RendererSoft::ProcessFaceFrontBackCull() {
  for (auto &fh : render_ctx_.faces) {
    if (fh.discard) {
      continue;
    }

    glm::vec4 &v0 = render_ctx_.vertexes[fh.indices[0]].pos;
    glm::vec4 &v1 = render_ctx_.vertexes[fh.indices[1]].pos;
    glm::vec4 &v2 = render_ctx_.vertexes[fh.indices[2]].pos;

    glm::vec3 n = glm::cross(glm::vec3(v1 - v0), glm::vec3(v2 - v0));
    float area = glm::dot(n, glm::vec3(0, 0, 1));
    fh.front_facing = area > 0;

    if (cull_face_back && !render_ctx_.double_sided) {
      fh.discard = !fh.front_facing;  // discard back face
    }
  }
}

void RendererSoft::ProcessFaceRasterization() {
  // early depth test
  if (depth_test && early_z) {
    for (auto &fh : render_ctx_.faces) {
      if (!fh.discard) {
        TriangularRasterization(fh, true);
      }
    }

    thread_pool_.WaitTasksFinish();
  }

  for (auto &fh : render_ctx_.faces) {
    if (!fh.discard) {
      TriangularRasterization(fh, false);
    }
  }
  thread_pool_.WaitTasksFinish();
}

void RendererSoft::ProcessFaceWireframe(bool clip_space) {
  glm::u8vec4 color{255};

  for (auto &fh : render_ctx_.faces) {
    if (fh.discard) {
      continue;
    }

    for (int i = 0; i < 3; i++) {
      int idx0 = fh.indices[i];
      int idx1 = fh.indices[(i + 1) % 3];

      VertexHolder &v0 = render_ctx_.vertexes[idx0];
      VertexHolder &v1 = render_ctx_.vertexes[idx1];

      if (clip_space) {
        DrawLineFrustumClip(v0.pos, v0.clip_mask, v1.pos, v1.clip_mask, color);
      } else {
        graphic_.DrawLine(v0.pos, v1.pos, color);
      }
    }
  }
}

void RendererSoft::TriangularRasterization(FaceHolder &face_holder, bool early_z_pass) {
  glm::aligned_vec4 screen_pos[3];
  for (int i = 0; i < 3; i++) {
    VertexHolder &vh = render_ctx_.vertexes[face_holder.indices[i]];
    screen_pos[i] = vh.pos;
  }

  BoundingBox bounds = VertexBoundingBox(screen_pos, width_, height_);
  bounds.min -= 1.f;

  auto block_size = (float) raster_block_size_;
  int block_cnt_x = (int) ((bounds.max.x - bounds.min.x + block_size - 1.f) / block_size);
  int block_cnt_y = (int) ((bounds.max.y - bounds.min.y + block_size - 1.f) / block_size);

  for (int block_y = 0; block_y < block_cnt_y; block_y++) {
    for (int block_x = 0; block_x < block_cnt_x; block_x++) {
      thread_pool_.PushTask([&, bounds, block_size, block_x, block_y](int thread_id) {
        // init pixel quad
        PixelQuadContext pixel_quad(render_ctx_.varyings_aligned_size);
        pixel_quad.frag_shader = shader_context_.fragment_shader->clone();
        pixel_quad.front_facing = face_holder.front_facing;

        glm::aligned_vec4 *screen_pos = pixel_quad.screen_pos;

        for (int i = 0; i < 3; i++) {
          VertexHolder &vh = render_ctx_.vertexes[face_holder.indices[i]];
          screen_pos[i] = vh.pos;
          pixel_quad.varying_in_ptr[i] = vh.varyings;
          pixel_quad.clip_z[i] = (depth_range.sum - vh.pos.z) * vh.pos.w;  // [far, near] -> [near, far]
        }

        pixel_quad.vert_flat[0] = {screen_pos[2].x, screen_pos[1].x, screen_pos[0].x, 0.f};
        pixel_quad.vert_flat[1] = {screen_pos[2].y, screen_pos[1].y, screen_pos[0].y, 0.f};
        pixel_quad.vert_flat[2] = {screen_pos[0].z, screen_pos[1].z, screen_pos[2].z, 0.f};
        pixel_quad.vert_flat[3] = {screen_pos[0].w, screen_pos[1].w, screen_pos[2].w, 0.f};

        // block rasterization
        int block_start_x = (int) (bounds.min.x + block_x * block_size);
        int block_start_y = (int) (bounds.min.y + block_y * block_size);
        for (int y = block_start_y + 1; y < block_start_y + block_size && y <= bounds.max.y; y += 2) {
          for (int x = block_start_x + 1; x < block_start_x + block_size && x <= bounds.max.x; x += 2) {
            pixel_quad.Init(x, y);
            PixelQuadBarycentric(pixel_quad, early_z_pass);
          }
        }
      });
    }
  }
}

void RendererSoft::PixelQuadBarycentric(PixelQuadContext &quad, bool early_z_pass) {
  glm::aligned_vec4 *vert = quad.vert_flat;
  glm::aligned_vec4 &v0 = quad.screen_pos[0];

  // barycentric
  for (auto &pixel : quad.pixels) {
    pixel.inside = graphic_.Barycentric(vert, v0, pixel.position, pixel.barycentric);
  }

  // skip total outside quad
  if (!quad.Inside()) {
    return;
  }

  // barycentric correction
  BarycentricCorrect(quad);

  // early z pass : depth test & write depth
  if (early_z_pass) {
    for (auto &pixel : quad.pixels) {
      if (!pixel.inside) {
        continue;
      }
      int pos_x = (int) pixel.position.x;
      int pos_y = (int) pixel.position.y;
      DepthTestPoint(pos_x, pos_y, pixel.position.z);
    }
    return;
  }

  // if early z enabled, perform depth test (Depth_EQUAL) before shading
  if (depth_test && early_z) {
    bool quad_pass = false;
    for (auto &pixel : quad.pixels) {
      if (!pixel.inside) {
        continue;
      }
      auto pos_x = (size_t) pixel.position.x;
      auto pos_y = (size_t) pixel.position.y;
      pixel.early_depth_passed = DepthTest(pixel.position.z, *fbo_.depth->Get(pos_x, pos_y), Depth_EQUAL);
      quad_pass = quad_pass || pixel.early_depth_passed;
    }
    if (!quad_pass) {
      return;
    }
  }

  // varying interpolate
  // note: all quad pixels should perform varying interpolate to enable varying partial derivative
  for (auto &pixel : quad.pixels) {
    VaryingInterpolate((float *) pixel.varyings_out, quad.varying_in_ptr,
                       render_ctx_.varyings_size, pixel.barycentric);
  }

  // pixel shading
  for (auto &pixel : quad.pixels) {
    glm::vec4 &pos = pixel.position;
    if (!pixel.inside || !pixel.early_depth_passed) {
      continue;
    }

    // fragment shader
    quad.frag_shader->varyings = (BaseShaderVaryings *) pixel.varyings_out;
    quad.frag_shader->quad_ctx = &quad;
    PixelShading(pos, quad.front_facing, quad.frag_shader.get());
  }
}

void RendererSoft::BarycentricCorrect(PixelQuadContext &quad) {
  glm::aligned_vec4 *vert = quad.vert_flat;
#ifdef SOFTGL_SIMD_OPT
  __m128 m_clip_z = _mm_load_ps(&quad.clip_z.x);
  __m128 m_screen_z = _mm_load_ps(&vert[2].x);
  __m128 m_screen_w = _mm_load_ps(&vert[3].x);
  for (auto &pixel : quad.pixels) {
    auto &bc = pixel.barycentric;
    __m128 m_bc = _mm_load_ps(&bc.x);

    // barycentric correction
    m_bc = _mm_div_ps(m_bc, m_clip_z);
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
    bc /= quad.clip_z;
    bc /= (bc.x + bc.y + bc.z);

    // interpolate z, w
    pixel.position.z = glm::dot(vert[2], bc);
    pixel.position.w = glm::dot(vert[3], bc);
  }
#endif
}

void RendererSoft::PixelShading(glm::vec4 &screen_pos, bool front_facing, BaseFragmentShader *frag_shader) {
  auto pos_x = (int) screen_pos.x;
  auto pos_y = (int) screen_pos.y;

  // fragment shader
  frag_shader->gl_FragCoord = glm::vec4(screen_pos.x, screen_pos.y, screen_pos.z, 1.0f / screen_pos.w);
  frag_shader->gl_FrontFacing = front_facing;
  frag_shader->shader_main();
  if (frag_shader->discard) {
    return;
  }

  // frag color
  glm::vec4 color = glm::clamp(frag_shader->gl_FragColor, 0.f, 1.f);
  if (blend_enabled && render_ctx_.alpha_blend) {
    glm::vec4 &src_color = color;
    glm::vec4 dst_color = glm::vec4(*fbo_.color->Get(pos_x, pos_y)) / 255.f;
    color = src_color * (src_color.a) + dst_color * (1.f - src_color.a);
  }

  if ((depth_test && early_z) || DepthTestPoint(pos_x, pos_y, frag_shader->gl_FragDepth)) {
    DrawFramePoint(pos_x, pos_y, color * 255.f);
  }
}

void RendererSoft::DrawFramePointWithDepth(int x, int y, float depth, const glm::u8vec4 &color) {
  if (DepthTestPoint(x, y, depth)) {
    DrawFramePoint(x, y, color);
  }
}

void RendererSoft::DrawFramePoint(int x, int y, const glm::u8vec4 &color) {
  fbo_.color->Set(x, y, color);
}

bool RendererSoft::DepthTestPoint(int x, int y, float depth) {
  if (!depth_test) {
    return true;
  }

  float *z_ptr = fbo_.depth->Get(x, y);
  if (z_ptr && DepthTest(depth, *z_ptr, depth_func)) {
    if (depth_mask) {
      *z_ptr = depth;
    }
    return true;
  }
  return false;
}

void RendererSoft::DrawLineFrustumClip(glm::vec4 p0, int mask0, glm::vec4 p1, int mask1, const glm::u8vec4 &color) {
  bool full_clip = false;
  float t0 = 0.f;
  float t1 = 1.f;

  int mask = mask0 | mask1;
  if (mask != 0) {
    for (int i = 0; i < 6; i++) {
      if (mask & FrustumClipMaskArray[i]) {
        float d0 = glm::dot(FrustumClipPlane[i], p0);
        float d1 = glm::dot(FrustumClipPlane[i], p1);

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
    return;
  }

  if (mask0) {
    p0 = glm::mix(p0, p1, t0);
  }

  if (mask1) {
    p1 = glm::mix(p0, p1, t1);
  }

  PerspectiveDivide(p0);
  PerspectiveDivide(p1);

  ViewportTransform(p0);
  ViewportTransform(p1);

  graphic_.DrawLine(p0, p1, color);
}

glm::vec4 RendererSoft::Viewport(float x, float y, float w, float h) {
  glm::vec4 m;
  m.x = w / 2.f;
  m.y = h / 2.f;
  m.z = x + m.x;
  m.w = y + m.y;
  return m;
}

void RendererSoft::PerspectiveDivide(glm::vec4 &pos) {
  float inv_w = 1.0f / pos.w;
  pos.x *= inv_w;
  pos.y *= inv_w;
  pos.z *= inv_w;
}

void RendererSoft::ViewportTransform(glm::vec4 &pos) const {
  pos.x = pos.x * viewport_.x + viewport_.z;
  pos.y = pos.y * viewport_.y + viewport_.w;

  // Reversed-Z  [-1, 1] -> [far, near]
  pos.z = 0.5f * (depth_range.sum - depth_range.diff * pos.z);
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

BoundingBox RendererSoft::VertexBoundingBox(glm::vec4 *vert, size_t w, size_t h) {
  float minX = std::min(std::min(vert[0].x, vert[1].x), vert[2].x);
  float minY = std::min(std::min(vert[0].y, vert[1].y), vert[2].y);
  float maxX = std::max(std::max(vert[0].x, vert[1].x), vert[2].x);
  float maxY = std::max(std::max(vert[0].y, vert[1].y), vert[2].y);

  minX = std::max(minX - 0.5f, 0.f);
  minY = std::max(minY - 0.5f, 0.f);
  maxX = std::min(maxX + 0.5f, (float) w - 1.f);
  maxY = std::min(maxY + 0.5f, (float) h - 1.f);

  auto min = glm::vec3(minX, minY, 0.f);
  auto max = glm::vec3(maxX, maxY, 0.f);
  return {min, max};
}

void RendererSoft::VaryingInterpolate(float *out_vary,
                                      const float *in_varyings[],
                                      size_t elem_cnt,
                                      glm::aligned_vec4 &bc) {
  const float *in_vary0 = in_varyings[0];
  const float *in_vary1 = in_varyings[1];
  const float *in_vary2 = in_varyings[2];

#ifdef SOFTGL_SIMD_OPT
  assert(PTR_ADDR(in_vary0) % SOFTGL_ALIGNMENT == 0);
  assert(PTR_ADDR(in_vary1) % SOFTGL_ALIGNMENT == 0);
  assert(PTR_ADDR(in_vary2) % SOFTGL_ALIGNMENT == 0);
  assert(PTR_ADDR(out_vary) % SOFTGL_ALIGNMENT == 0);

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
      _mm256_store_ps(out_vary + idx, sum);
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
      _mm_store_ps(out_vary + idx, sum);
    }
  }

  for (; idx < elem_cnt; idx++) {
    out_vary[idx] = 0;
    out_vary[idx] += *(in_vary0 + idx) * bc[0];
    out_vary[idx] += *(in_vary1 + idx) * bc[1];
    out_vary[idx] += *(in_vary2 + idx) * bc[2];
  }
#else
  for (int i = 0; i < elem_cnt; i++) {
    out_vary[i] = glm::dot(bc, glm::vec4(*(in_vary0 + i), *(in_vary1 + i), *(in_vary2 + i), 0.f));
  }
#endif
}

}