/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "render/vertex.h"
#include "render/bounding_box.h"
#include "material.h"

namespace SoftGL {
namespace View {

struct Vertex {
  glm::vec3 a_position;
  glm::vec2 a_texCoord;
  glm::vec3 a_normal;
  glm::vec3 a_tangent;
};

struct ModelVertexes : VertexArray {
  PrimitiveType primitive_type;
  size_t primitive_cnt = 0;
  std::vector<Vertex> vertexes;
  std::vector<int32_t> indices;
  std::shared_ptr<VertexArrayObject> vao = nullptr;

  void UpdateVertexes() const {
    if (vao) {
      vao->UpdateVertexData(vertexes_buffer, vertexes_buffer_length);
    }
  };

  void InitVertexes() {
    vertexes_desc.resize(4);
    vertexes_desc[0] = {3, sizeof(Vertex), offsetof(Vertex, a_position)};
    vertexes_desc[1] = {2, sizeof(Vertex), offsetof(Vertex, a_texCoord)};
    vertexes_desc[2] = {3, sizeof(Vertex), offsetof(Vertex, a_normal)};
    vertexes_desc[3] = {3, sizeof(Vertex), offsetof(Vertex, a_tangent)};

    vertexes_buffer = vertexes.empty() ? nullptr : (uint8_t *) &vertexes[0];
    vertexes_buffer_length = vertexes.size() * sizeof(Vertex);

    indices_buffer = indices.empty() ? nullptr : &indices[0];
    indices_buffer_length = indices.size() * sizeof(int32_t);
  }
};

struct ModelPoints : ModelVertexes {
  float point_size;
  BaseColorMaterial material;
};

struct ModelLines : ModelVertexes {
  float line_width;
  BaseColorMaterial material;
};

struct ModelMesh : ModelVertexes {
  BoundingBox aabb;
  BaseColorMaterial material_wireframe;
  TexturedMaterial material_textured;
};

struct ModelSkybox : ModelVertexes {
  std::unordered_map<std::string, SkyboxMaterial> material_cache;
  SkyboxMaterial *material = nullptr;
};

struct ModelNode {
  glm::mat4 transform;
  std::vector<ModelMesh> meshes;
  std::vector<ModelNode> children;
};

struct Model {
  std::string res_dir;

  ModelNode root_node;
  BoundingBox root_aabb;

  size_t mesh_cnt = 0;
  size_t primitive_cnt = 0;
  size_t vertex_cnt = 0;
};

struct DemoScene {
  std::shared_ptr<Model> model;
  ModelLines world_axis;
  ModelPoints point_light;
  ModelSkybox skybox;
};

}
}
