/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "renderer_soft.h"
#include "base/hash_utils.h"
#include "framebuffer_soft.h"
#include "texture_soft.h"
#include "uniform_soft.h"
#include "shader_program_soft.h"
#include "vertex_soft.h"

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
  fbo_color_ = fbo_->GetColorBuffer();
  fbo_depth_ = fbo_->GetDepthBuffer();
}

void RendererSoft::SetViewPort(int x, int y, int width, int height) {
  viewport_.x = (float) x;
  viewport_.y = (float) y;
  viewport_.width = (float) width;
  viewport_.height = (float) height;

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
  if (state.color_flag) {
    if (fbo_color_) {
      fbo_color_->SetAll(glm::u8vec4(state.clear_color.r * 255,
                                     state.clear_color.g * 255,
                                     state.clear_color.b * 255,
                                     state.clear_color.a * 255));
    }
  }

  if (state.depth_flag) {
    if (fbo_depth_) {
      fbo_depth_->SetAll(viewport_.depth_far);
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
  vertexes_.resize(vao_->vertex_cnt);
  auto &builtin = shader_program_->GetShaderBuiltin();
  uint8_t *vertex_ptr = vao_->vertexes.data();

  for (int idx = 0; idx < vao_->vertex_cnt; idx++) {
    shader_program_->BindVertexAttributes(vertex_ptr);
    shader_program_->ExecVertexShader();

    VertexHolder &holder = vertexes_[idx];
    holder.index = idx;
    holder.discard = false;
    holder.position = builtin.Position;

    vertex_ptr += vao_->vertex_stride;
  }
}

void RendererSoft::ProcessPrimitiveAssembly() {
  switch (primitive_type_) {
    case Primitive_POINT: {
      primitives_.resize(vao_->indices_cnt);
      for (int idx = 0; idx < primitives_.size(); idx++) {
        auto &point = primitives_[idx];
        point.vertexes[0] = &vertexes_[vao_->indices[idx]];
        point.discard = false;
      }
      break;
    }
    case Primitive_LINE: {
      primitives_.resize(vao_->indices_cnt / 2);
      for (int idx = 0; idx < primitives_.size(); idx++) {
        auto &line = primitives_[idx];
        line.vertexes[0] = &vertexes_[vao_->indices[idx * 2]];
        line.vertexes[1] = &vertexes_[vao_->indices[idx * 2 + 1]];
        line.discard = false;
      }
      break;
    }
    case Primitive_TRIANGLE: {
      primitives_.resize(vao_->indices_cnt / 3);
      for (int idx = 0; idx < primitives_.size(); idx++) {
        auto &triangle = primitives_[idx];
        triangle.vertexes[0] = &vertexes_[vao_->indices[idx * 3]];
        triangle.vertexes[1] = &vertexes_[vao_->indices[idx * 3 + 1]];
        triangle.vertexes[2] = &vertexes_[vao_->indices[idx * 3 + 2]];
        triangle.discard = false;
      }
      break;
    }
  }
}

void RendererSoft::ProcessClipping() {
  // TODO clipping

  // update vertexes discard flag
  for (auto &primitive : primitives_) {
    if (primitive.discard) {
      continue;
    }
    switch (primitive_type_) {
      case Primitive_POINT:
        primitive.vertexes[0]->discard = false;
        break;
      case Primitive_LINE:
        primitive.vertexes[0]->discard = false;
        primitive.vertexes[1]->discard = false;
        break;
      case Primitive_TRIANGLE:
        primitive.vertexes[0]->discard = false;
        primitive.vertexes[1]->discard = false;
        primitive.vertexes[2]->discard = false;
        break;
    }
  }
}

void RendererSoft::ProcessPerspectiveDivide() {
  for (auto &vertex : vertexes_) {
    if (vertex.discard) {
      continue;
    }
    auto &pos = vertex.position;
    float inv_w = 1.0f / pos.w;
    pos.x *= inv_w;
    pos.y *= inv_w;
    pos.z *= inv_w;
  }
}

void RendererSoft::ProcessViewportTransform() {
  for (auto &vertex : vertexes_) {
    if (vertex.discard) {
      continue;
    }

    vertex.position *= viewport_.inner_p;
    vertex.position += viewport_.inner_o;
  }
}

void RendererSoft::ProcessFaceCulling() {
  if (primitive_type_ != Primitive_TRIANGLE) {
    return;
  }

  for (auto &triangle : primitives_) {
    glm::vec4 &v0 = triangle.vertexes[0]->position;
    glm::vec4 &v1 = triangle.vertexes[1]->position;
    glm::vec4 &v2 = triangle.vertexes[2]->position;

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
    case Primitive_POINT: {
      for (auto &primitive : primitives_) {
        auto &vert = primitive.vertexes;
        if (primitive.discard) {
          continue;
        }
        RasterizationPoint(vert[0]->position, render_state_->point_size);
      }
      break;
    }
    case Primitive_LINE: {
      for (auto &primitive : primitives_) {
        auto &vert = primitive.vertexes;
        if (primitive.discard) {
          continue;
        }
        RasterizationLine(vert[0]->position, vert[1]->position, render_state_->line_width);
      }
      break;
    }
    case Primitive_TRIANGLE: {
      for (auto &primitive : primitives_) {
        auto &vert = primitive.vertexes;
        if (primitive.discard) {
          continue;
        }
        switch (render_state_->polygon_mode) {
          case Polygon_POINT:
            // TODO remove duplicate points
            for (int i = 0; i < 3; i++) {
              RasterizationPoint(vert[i]->position, render_state_->point_size);
            }
            break;
          case Polygon_LINE:
            // TODO remove duplicate lines
            for (int i = 0; i < 3; i++) {
              RasterizationLine(vert[i]->position, vert[(i + 1) % 3]->position, render_state_->line_width);
            }
            break;
          case Polygon_FILL:
            RasterizationTriangle(vert[0]->position, vert[1]->position, vert[2]->position, primitive.front_facing);
            break;
        }
      }
      break;
    }
  }
}

void RendererSoft::ProcessFragmentShader(glm::vec4 &screen_pos, bool front_facing) {
  auto pos_x = (int) screen_pos.x;
  auto pos_y = (int) screen_pos.y;

  auto &builtin = shader_program_->GetShaderBuiltin();
  builtin.FragCoord = screen_pos;
  builtin.FragCoord.w = 1.f / builtin.FragCoord.w;
  builtin.FrontFacing = front_facing;
  builtin.FragDepth = builtin.FragCoord.z;   // default depth

  shader_program_->ExecFragmentShader();
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

  float *z_ptr = fbo_depth_->Get(x, y);
  if (z_ptr && DepthTest(depth, *z_ptr, render_state_->depth_func)) {
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
    // TODO render_state_.blend_src/blend_dst
    color = src_color * (src_color.a) + dst_color * (1.f - src_color.a);
  }
}

void RendererSoft::RasterizationPoint(glm::vec4 &pos, float point_size) {
  float left = pos.x - point_size / 2.f + 0.5f;
  float right = left + point_size;
  float top = pos.y - point_size / 2.f + 0.5f;
  float bottom = top + point_size;

  glm::vec4 screen_pos = pos;
  for (int x = (int) left; x < (int) right; x++) {
    for (int y = (int) top; y < (int) bottom; y++) {
      screen_pos.x = (float) x;
      screen_pos.y = (float) y;
      ProcessFragmentShader(screen_pos, true);
    }
  }
}

void RendererSoft::RasterizationLine(glm::vec4 &pos0, glm::vec4 &pos1, float line_width) {
  auto &v0 = pos0;
  auto &v1 = pos1;

  int x0 = (int) v0.x, y0 = (int) v0.y;
  int x1 = (int) v1.x, y1 = (int) v1.y;

  float z0 = v0.z;
  float z1 = v1.z;

  float w0 = v0.w;
  float w1 = v1.w;

  bool steep = false;
  if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
    std::swap(x0, y0);
    std::swap(x1, y1);
    steep = true;
  }
  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
    std::swap(z0, z1);
  }
  int dx = x1 - x0;
  int dy = y1 - y0;
  float dz = (x1 == x0) ? 0 : (z1 - z0) / (float) (x1 - x0);
  float dw = (x1 == x0) ? 0 : (w1 - w0) / (float) (x1 - x0);

  int error = 0;
  int dError = 2 * std::abs(dy);

  int y = y0;
  float z = z0;
  float w = w0;

  glm::vec4 screen_pos(0, 0, z, w);
  for (int x = x0; x <= x1; x++) {
    z += dz;
    w += dw;

    screen_pos.x = (float) x;
    screen_pos.y = (float) y;
    screen_pos.z = (float) z;
    if (steep) {
      std::swap(screen_pos.x, screen_pos.y);
    }
    RasterizationPoint(screen_pos, line_width);

    error += dError;
    if (error > dx) {
      y += (y1 > y0 ? 1 : -1);
      error -= 2 * dx;
    }
  }
}

void RendererSoft::RasterizationTriangle(glm::vec4 &pos0, glm::vec4 &pos1, glm::vec4 &pos2, bool front_facing) {

}

glm::u8vec4 RendererSoft::GetFrameColor(int x, int y) {
  return *fbo_color_->Get(x, y);
}

void RendererSoft::SetFrameColor(int x, int y, const glm::u8vec4 &color) {
  fbo_color_->Set(x, y, color);
}

bool RendererSoft::DepthTest(float &a, float &b, DepthFunc func) {
  switch (func) {
    case Depth_NEVER:
      return false;
    case Depth_LESS:
      return a < b;
    case Depth_EQUAL:
      return std::fabs(a - b) <= std::numeric_limits<float>::epsilon();
    case Depth_LEQUAL:
      return a <= b;
    case Depth_GREATER:
      return a > b;
    case Depth_NOTEQUAL:
      return std::fabs(a - b) > std::numeric_limits<float>::epsilon();
    case Depth_GEQUAL:
      return a >= b;
    case Depth_ALWAYS:
      return true;
  }
  return a < b;
}

}
