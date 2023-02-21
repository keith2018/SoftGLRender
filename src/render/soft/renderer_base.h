/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/memory_utils.h"
#include "render/soft/shader_program_soft.h"

namespace SoftGL {

struct Viewport {
  float x;
  float y;
  float width;
  float height;
  float depth_near;
  float depth_far;

  // ref: https://registry.khronos.org/vulkan/specs/1.0/html/chap24.html#vertexpostproc-viewport
  glm::vec4 inner_o;
  glm::vec4 inner_p;
};

struct VertexHolder {
  bool discard = false;
  size_t index = 0;

  void *vertex = nullptr;
  float *varyings = nullptr;

  int clip_mask = 0;
  glm::vec4 clip_pos = glm::vec4(0.f);  // clip space position
  glm::vec4 src_pos = glm::vec4(0.f);   // screen space position

  std::shared_ptr<uint8_t> vertex_holder = nullptr;
  std::shared_ptr<float> varyings_holder = nullptr;
};

struct PrimitiveHolder {
  bool discard = false;
  bool front_facing = true;
  size_t indices[3] = {0, 0, 0};
};

struct PixelContext {
  bool inside = false;
  float *varyings_frag = nullptr;
  glm::aligned_vec4 position = glm::aligned_vec4(0.f);
  glm::aligned_vec4 barycentric = glm::aligned_vec4(0.f);
};

class PixelQuadContext {
 public:
  void SetVaryingsSize(size_t size) {
    if (varyings_aligned_cnt_ != size) {
      varyings_aligned_cnt_ = size;
      varyings_pool_ = MemoryUtils::MakeAlignedBuffer<float>(4 * varyings_aligned_cnt_);
      for (int i = 0; i < 4; i++) {
        pixels[i].varyings_frag = varyings_pool_.get() + i * varyings_aligned_cnt_;
      }
    }
  }

  void Init(float x, float y) {
    pixels[0].position.x = x;
    pixels[0].position.y = y;

    pixels[1].position.x = x + 1;
    pixels[1].position.y = y;

    pixels[2].position.x = x;
    pixels[2].position.y = y + 1;

    pixels[3].position.x = x + 1;
    pixels[3].position.y = y + 1;
  }

  bool QuadInside() {
    return pixels[0].inside || pixels[1].inside || pixels[2].inside || pixels[3].inside;
  }

 public:
  /**
   *   p2--p3
   *   |   |
   *   p0--p1
   */
  PixelContext pixels[4];

  // triangle vertex screen space position
  glm::aligned_vec4 vert_pos[3];
  glm::aligned_vec4 vert_pos_flat[4];

  // triangle barycentric correct factor
  glm::aligned_vec4 vert_bc_factor = glm::aligned_vec4(0.f, 0.f, 0.f, 1.f);

  // triangle vertex shader varyings
  const float *vert_varyings[3] = {nullptr, nullptr, nullptr};

  // triangle Facing
  bool front_facing = true;

  // shader program
  std::shared_ptr<ShaderProgramSoft> shader_program = nullptr;

 private:
  size_t varyings_aligned_cnt_ = 0;
  std::shared_ptr<float> varyings_pool_ = nullptr;
};

}