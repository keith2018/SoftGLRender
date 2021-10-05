/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Common.h"
#include "Texture.h"
#include "BoundingBox.h"

namespace SoftGL {

struct ModelPoints {
  size_t point_cnt = 0;
  std::vector<Vertex> vertexes;
  std::vector<glm::vec3> colors;
};

struct ModelLines {
  size_t line_cnt = 0;
  std::vector<Vertex> vertexes;
  std::vector<int> indices;
};

struct ModelMesh {
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