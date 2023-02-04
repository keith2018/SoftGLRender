/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "renderer_soft.h"
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

std::shared_ptr<UniformSampler> RendererSoft::CreateUniformSampler(const std::string &name) {
  return std::make_shared<UniformSamplerSoft>(name);
}

// pipeline
void RendererSoft::SetFrameBuffer(FrameBuffer &frame_buffer) {
  fbo_ = dynamic_cast<FrameBufferSoft *>(&frame_buffer);
  fbo_color_ = fbo_->GetColorBuffer();
  fbo_depth_ = fbo_->GetDepthBuffer();
}

void RendererSoft::SetViewPort(int x, int y, int width, int height) {
  viewport_.x = (float) width / 2.f;
  viewport_.y = (float) height / 2.f;
  viewport_.z = (float) x + viewport_.x;
  viewport_.w = (float) y + viewport_.y;
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
      fbo_depth_->SetAll(depth_range_.near);
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
  ProcessRasterization();
}

void RendererSoft::SetDepthRange(float near, float far) {
  depth_range_.near = near;
  depth_range_.far = far;
  depth_range_.diff = far - near;
  depth_range_.sum = far + near;
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
    holder.position = builtin.Position;

    vertex_ptr += vao_->vertex_stride;
  }
}

void RendererSoft::ProcessPrimitiveAssembly() {
  switch (primitive_type_) {
    case Primitive_POINTS: {
      primitive_points_.resize(vao_->indices_cnt);
      for (int idx = 0; idx < primitive_points_.size(); idx++) {
        auto &point = primitive_points_[idx];
        point.vertexes[0] = &vertexes_[vao_->indices[idx]];
      }
      break;
    }
    case Primitive_LINES: {
      primitive_lines_.resize(vao_->indices_cnt / 2);
      for (int idx = 0; idx < primitive_lines_.size(); idx++) {
        auto &line = primitive_lines_[idx];
        line.vertexes[0] = &vertexes_[vao_->indices[idx * 2]];
        line.vertexes[1] = &vertexes_[vao_->indices[idx * 2 + 1]];
      }
      break;
    }
    case Primitive_TRIANGLES: {
      primitive_triangles_.resize(vao_->indices_cnt / 3);
      for (int idx = 0; idx < primitive_triangles_.size(); idx++) {
        auto &triangle = primitive_triangles_[idx];
        triangle.vertexes[0] = &vertexes_[vao_->indices[idx * 3]];
        triangle.vertexes[1] = &vertexes_[vao_->indices[idx * 3 + 1]];
        triangle.vertexes[2] = &vertexes_[vao_->indices[idx * 3 + 2]];
      }
      // Face Culling
      ProcessFaceCulling();
      break;
    }
  }
}

void RendererSoft::ProcessFaceCulling() {
  for (auto &triangle : primitive_triangles_) {
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

void RendererSoft::ProcessClipping() {
  // TODO

  // update vertexes discard flag
  switch (primitive_type_) {
    case Primitive_POINTS: {
      for (auto &point : primitive_points_) {
        if (!point.discard) {
          point.vertexes[0]->discard = false;
        }
      }
      break;
    }
    case Primitive_LINES: {
      for (auto &line : primitive_lines_) {
        if (!line.discard) {
          line.vertexes[0]->discard = false;
          line.vertexes[1]->discard = false;
        }
      }
      break;
    }
    case Primitive_TRIANGLES: {
      for (auto &triangle : primitive_triangles_) {
        if (!triangle.discard) {
          triangle.vertexes[0]->discard = false;
          triangle.vertexes[1]->discard = false;
          triangle.vertexes[2]->discard = false;
        }
      }
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
    auto &pos = vertex.position;
    pos.x = pos.x * viewport_.x + viewport_.z;
    pos.y = pos.y * viewport_.y + viewport_.w;

    // Reversed-Z  [-1, 1] -> [far, near]
    pos.z = 0.5f * (depth_range_.sum - depth_range_.diff * pos.z);
  }
}

void RendererSoft::ProcessRasterization() {
  switch (primitive_type_) {
    case Primitive_POINTS:
      RasterizationPoint();
      break;
    case Primitive_LINES:
      RasterizationLine();
      break;
    case Primitive_TRIANGLES:
      RasterizationTriangle();
      break;
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

  // depth test
  if (!ProcessDepthTest(pos_x, pos_y, builtin.FragDepth)) {
    return;
  }

  glm::vec4 color = glm::clamp(builtin.FragColor, 0.f, 1.f);

  // blending
  if (render_state_->blend) {
    glm::vec4 &src_color = color;
    glm::vec4 dst_color = glm::vec4(GetFrameColor(pos_x, pos_y)) / 255.f;
    // TODO render_state_.blend_src/blend_dst
    color = src_color * (src_color.a) + dst_color * (1.f - src_color.a);
  }

  // write final pixel color to fbo
  SetFrameColor(pos_x, pos_y, color * 255.f);
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

void RendererSoft::RasterizationPoint() {
  for (auto &point : primitive_points_) {
    if (point.discard) {
      continue;
    }

    auto &pos = point.vertexes[0]->position;
    auto &size = render_state_->point_size;

    float left = pos.x - size / 2.f + 0.5f;
    float right = left + size;
    float top = pos.y - size / 2.f + 0.5f;
    float bottom = top + size;

    glm::vec4 screen_pos = pos;
    for (int x = (int) left; x < (int) right; x++) {
      for (int y = (int) top; y < (int) bottom; y++) {
        screen_pos.x = (float) x;
        screen_pos.y = (float) y;
        ProcessFragmentShader(screen_pos, true);
      }
    }
  }
}

void RendererSoft::RasterizationLine() {
  for (auto &line : primitive_lines_) {
    if (line.discard) {
      continue;
    }

    // line width not support right now
    auto &v0 = line.vertexes[0]->position;
    auto &v1 = line.vertexes[1]->position;

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
      if (steep) {
        std::swap(screen_pos.x, screen_pos.y);
      }
      ProcessFragmentShader(screen_pos, true);

      error += dError;
      if (error > dx) {
        y += (y1 > y0 ? 1 : -1);
        error -= 2 * dx;
      }
    }
  }
}

void RendererSoft::RasterizationTriangle() {

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
