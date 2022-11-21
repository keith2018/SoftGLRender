/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "common.h"
#include "texture.h"
#include "bounding_box.h"

namespace SoftGL {

class VertexHandler {
};

struct ModelBase {
  std::shared_ptr<VertexHandler> handle = nullptr;
};

struct ModelPoints : ModelBase {
  size_t point_cnt = 0;
  std::vector<Vertex> vertexes;
  std::vector<int> indices;
  glm::vec4 point_color;
  float point_size;
};

struct ModelLines : ModelBase {
  size_t line_cnt = 0;
  std::vector<Vertex> vertexes;
  std::vector<int> indices;
  glm::vec4 line_color;
  float line_width;
};

struct ModelMesh : ModelBase {
  size_t idx = 0;
  size_t face_cnt = 0;
  std::vector<Vertex> vertexes;
  std::vector<int> indices;
  std::unordered_map<int, Texture> textures;
  BoundingBox bounding_box{};

  // material param
  ShadingType shading_type = ShadingType_UNKNOWN;
  AlphaMode alpha_mode = Alpha_Opaque;
  float alpha_cutoff = 0.5f;
  bool double_sided = false;

  ModelMesh(ModelMesh &&o) = default;
  ModelMesh() = default;
};

struct ModelNode {
  glm::mat4 transform;
  std::vector<ModelMesh> meshes;
  std::vector<ModelNode> children;
};

}