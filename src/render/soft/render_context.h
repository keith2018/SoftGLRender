/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/common.h"
#include "utils/memory_utils.h"

namespace SoftGL {

struct FaceHolder {
  int indices[3]{-1, -1, -1};
  bool discard = false;
  bool front_facing = true;
};

struct VertexHolder {
  size_t index = 0;
  Vertex *vertex = nullptr;
  float *varyings = nullptr;

  glm::vec4 pos{0};
  int clip_mask = 0;

  std::shared_ptr<float> varyings_append = nullptr;
  std::shared_ptr<Vertex> vertex_append = nullptr;
};

struct RenderContext {
  void Prepare(ModelPoints *points) {
    vertexes.resize(points->vertexes.size());
    for (int i = 0; i < vertexes.size(); i++) {
      vertexes[i].index = i;
      vertexes[i].vertex = &points->vertexes[i];
      vertexes[i].pos = glm::vec4(0);
      vertexes[i].clip_mask = 0;
    }

    varyings_size = 0;
    varyings_aligned_size = 0;
    varyings_pool = nullptr;
    faces.resize(0);
  }

  void Prepare(ModelLines *lines) {
    vertexes.resize(lines->vertexes.size());
    for (int i = 0; i < vertexes.size(); i++) {
      vertexes[i].index = i;
      vertexes[i].vertex = &lines->vertexes[i];
      vertexes[i].pos = glm::vec4(0);
      vertexes[i].clip_mask = 0;
    }

    varyings_size = 0;
    varyings_aligned_size = 0;
    varyings_pool = nullptr;
    faces.resize(0);
  }

  void Prepare(ModelMesh *mesh, size_t vary_size = 0) {
    vertexes.resize(mesh->vertexes.size());
    for (int i = 0; i < vertexes.size(); i++) {
      vertexes[i].index = i;
      vertexes[i].vertex = &mesh->vertexes[i];
      vertexes[i].pos = glm::vec4(0);
      vertexes[i].clip_mask = 0;
    }

    varyings_size = vary_size;
    varyings_aligned_size = VaryingsAlignedSize(varyings_size);
    varyings_pool = nullptr;
    if (varyings_aligned_size > 0) {
      varyings_pool = std::shared_ptr<float>((float *) MemoryUtils::AlignedMalloc(varyings_aligned_size * vertexes.size()),
                                             [](const float *ptr) { MemoryUtils::AlignedFree((void *) ptr); });

      for (int i = 0; i < vertexes.size(); i++) {
        vertexes[i].varyings = varyings_pool.get() + i * varyings_aligned_size / sizeof(float);
      }
    }

    faces.resize(mesh->primitive_cnt);
    for (int i = 0; i < faces.size(); i++) {
      for (int j = 0; j < 3; j++) {
        faces[i].indices[j] = mesh->indices[i * 3 + j];
        faces[i].discard = false;
      }
    }

    alpha_blend = (mesh->alpha_mode == Alpha_Blend);
    double_sided = mesh->double_sided;
  }

  VertexHolder VertexHolderInterpolate(VertexHolder *v0, VertexHolder *v1, float weight) const {
    VertexHolder ret;
    ret.vertex_append = std::make_shared<Vertex>();
    ret.vertex = ret.vertex_append.get();

    auto *vtf_ret = (float *) ret.vertex;
    auto *vtf_0 = (float *) v0->vertex;
    auto *vtf_1 = (float *) v1->vertex;
    for (int i = 0; i < sizeof(Vertex) / sizeof(float); i++) {
      vtf_ret[i] = glm::mix(vtf_0[i], vtf_1[i], weight);
    }
    ret.pos = glm::vec4(0);
    ret.clip_mask = 0;
    if (varyings_aligned_size > 0) {
      ret.varyings_append = std::shared_ptr<float>((float *) MemoryUtils::AlignedMalloc(varyings_aligned_size),
                                                   [](const float *ptr) { MemoryUtils::AlignedFree((void *) ptr); });
      ret.varyings = ret.varyings_append.get();
    }
    return ret;
  }

  inline static size_t VaryingsAlignedSize(size_t vary_size) {
    return SOFTGL_ALIGNMENT * std::ceil(vary_size * sizeof(float) / (float) SOFTGL_ALIGNMENT);
  }

  size_t varyings_size = 0;
  size_t varyings_aligned_size = 0;
  std::shared_ptr<float> varyings_pool = nullptr;

  std::vector<VertexHolder> vertexes;
  std::vector<FaceHolder> faces;

  bool alpha_blend = false;
  bool double_sided = false;
};

struct PixelContext {
  glm::aligned_vec4 position;
  glm::aligned_vec4 barycentric;
  float *varyings_out;
  bool inside = false;
  bool early_depth_passed = true;
};

struct PixelQuadContext {

  /**
   *   p2--p3
   *   |   |
   *   p0--p1
   */
  PixelContext pixels[4];
  std::shared_ptr<float> varyings_out_pool = nullptr;
  std::shared_ptr<BaseFragmentShader> frag_shader = nullptr;

  // Triangular vertex screen space position
  glm::aligned_vec4 screen_pos[3];
  glm::aligned_vec4 vert_flat[4];

  // Triangular vertex clip space z, clip_z = { v0.clip_z, v1.clip_z, v2.clip_z, 1.f}
  glm::aligned_vec4 clip_z{1.f};

  // Triangular vertex shader varyings
  const float *varying_in_ptr[3];

  // Triangular Facing
  bool front_facing = true;

  explicit PixelQuadContext(size_t varyings_aligned_size) {
    if (varyings_aligned_size > 0) {
      varyings_out_pool = std::shared_ptr<float>((float *) MemoryUtils::AlignedMalloc(4 * varyings_aligned_size),
                                                 [](const float *ptr) { MemoryUtils::AlignedFree((void *) ptr); });
      for (int i = 0; i < 4; i++) {
        pixels[i].varyings_out = varyings_out_pool.get() + i * varyings_aligned_size / sizeof(float);
      }
    }
  }

  void Init(int x, int y) {
    pixels[0].position.x = x;
    pixels[0].position.y = y;

    pixels[1].position.x = x + 1;
    pixels[1].position.y = y;

    pixels[2].position.x = x;
    pixels[2].position.y = y + 1;

    pixels[3].position.x = x + 1;
    pixels[3].position.y = y + 1;
  }

  bool Inside() {
    return pixels[0].inside || pixels[1].inside || pixels[2].inside || pixels[3].inside;
  }

  // diff = A - B
  inline float *DFDX_A() {
    return pixels[1].varyings_out;
  }

  inline float *DFDX_B() {
    return pixels[0].varyings_out;
  }

  // diff = A - B
  inline float *DFDY_A() {
    return pixels[2].varyings_out;
  }

  inline float *DFDY_B() {
    return pixels[0].varyings_out;
  }
};

}
