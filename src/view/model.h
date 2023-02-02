/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <memory>
#include <string>

#include "render/vertex.h"
#include "render/bounding_box.h"
#include "material.h"

namespace SoftGL {
namespace View {

struct ModelPoints : VertexArray {
  float point_size;
  BaseColorMaterial material;
};

struct ModelLines : VertexArray {
  float line_width;
  BaseColorMaterial material;
};

struct ModelMesh : VertexArray {
  BoundingBox aabb;
  BaseColorMaterial material_wireframe;
  TexturedMaterial material_textured;
};

struct ModelSkybox : VertexArray {
  SkyboxMaterial material;
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
