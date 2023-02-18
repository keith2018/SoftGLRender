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
  bool discard;
  size_t index;

  void *vertex;
  float *varyings;

  float clip_z;
  int clip_mask;
  glm::vec4 position;

  std::shared_ptr<uint8_t> vertex_holder;
  std::shared_ptr<float> varyings_holder;
};

struct PrimitiveHolder {
  bool discard;
  bool front_facing;
  size_t indices[3];
};

struct PixelContext {
  bool inside = false;
  float *varyings_frag;
  glm::aligned_vec4 position;
  glm::aligned_vec4 barycentric;
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
  glm::aligned_vec4 vert_bc_factor{1.f};

  // triangle vertex shader varyings
  const float *vert_varyings[3];

  // triangle Facing
  bool front_facing = true;

  // shader program
  std::shared_ptr<ShaderProgramSoft> shader_program = nullptr;

 private:
  size_t varyings_aligned_cnt_ = 0;
  std::shared_ptr<float> varyings_pool_;
};

}