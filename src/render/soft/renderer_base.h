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

  float depth_min;
  float depth_max;

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
  glm::vec4 scr_pos = glm::vec4(0.f);   // screen space position

  std::shared_ptr<uint8_t> vertex_holder = nullptr;
  std::shared_ptr<float> varyings_holder = nullptr;
};

struct PrimitiveHolder {
  bool discard = false;
  bool front_facing = true;
  size_t indices[3] = {0, 0, 0};
};

class SampleContext {
 public:
  bool inside = false;
  glm::ivec2 fbo_coord = glm::ivec2(0);
  glm::aligned_vec4 position = glm::aligned_vec4(0.f);
  glm::aligned_vec4 barycentric = glm::aligned_vec4(0.f);
};

class PixelContext {
 public:
  inline static glm::vec2 *GetSampleLocation4X() {
    static glm::vec2 location_4x[4] = {
        {0.375f, 0.875f},
        {0.875f, 0.625f},
        {0.125f, 0.375f},
        {0.625f, 0.125f},
    };
    return location_4x;
  }

  void Init(float x, float y, int sample_cnt = 1) {
    inside = false;
    sample_count = sample_cnt;
    coverage = 0;
    if (sample_count > 1) {
      samples.resize(sample_count + 1);  // store center sample at end
      if (sample_count == 4) {
        for (int i = 0; i < sample_count; i++) {
          samples[i].fbo_coord = glm::ivec2(x, y);
          samples[i].position = glm::vec4(GetSampleLocation4X()[i] + glm::vec2(x, y), 0.f, 0.f);
        }
        // pixel center
        samples[4].fbo_coord = glm::ivec2(x, y);
        samples[4].position = glm::vec4(x + 0.5f, y + 0.5f, 0.f, 0.f);
        sample_shading = &samples[4];
      } else {
        // not support
      }
    } else {
      samples.resize(1);
      samples[0].fbo_coord = glm::ivec2(x, y);
      samples[0].position = glm::vec4(x + 0.5f, y + 0.5f, 0.f, 0.f);
      sample_shading = &samples[0];
    }
  }

  bool InitCoverage() {
    if (sample_count > 1) {
      coverage = 0;
      inside = false;
      for (int i = 0; i < samples.size() - 1; i++) {
        if (samples[i].inside) {
          coverage++;
        }
      }
      inside = coverage > 0;
    } else {
      coverage = 1;
      inside = samples[0].inside;
    }
    return inside;
  }

  void InitShadingSample() {
    if (sample_shading->inside) {
      return;
    }
    for (auto &sample : samples) {
      if (sample.inside) {
        sample_shading = &sample;
        break;
      }
    }
  }

 public:
  bool inside = false;
  float *varyings_frag = nullptr;
  std::vector<SampleContext> samples;
  SampleContext *sample_shading = nullptr;
  int sample_count = 0;
  int coverage = 0;
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

  void Init(float x, float y, int sample_cnt = 1) {
    pixels[0].Init(x, y, sample_cnt);
    pixels[1].Init(x + 1, y, sample_cnt);
    pixels[2].Init(x, y + 1, sample_cnt);
    pixels[3].Init(x + 1, y + 1, sample_cnt);
  }

  bool CheckInside() {
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
  glm::aligned_vec4 vert_clip_z = glm::aligned_vec4(0.f, 0.f, 0.f, 1.f);

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
