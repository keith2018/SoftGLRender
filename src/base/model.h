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

enum SkyboxTextureType {
  Skybox_Cube,
  Skybox_Equirectangular,
};

struct SkyboxTexture {
  SkyboxTextureType type;
  Texture cube[6];  // +x, -x, +y, -y, +z, -z
  Texture equirectangular;
};

class RenderHandler {
};

struct ModelBase {
  size_t primitive_cnt = 0;
  std::vector<Vertex> vertexes;
  std::vector<int> indices;

  ShadingType shading_type = ShadingType_BASE_COLOR;
  std::shared_ptr<RenderHandler> render_handle = nullptr;
};

struct ModelPoints : ModelBase {
  glm::vec4 point_color;
  float point_size;
};

struct ModelLines : ModelBase {
  glm::vec4 line_color;
  float line_width;
};

struct ModelMesh : ModelBase {
  size_t idx = 0;
  std::unordered_map<TextureType, Texture, EnumClassHash> textures;
  BoundingBox bounding_box{};

  // material param
  AlphaMode alpha_mode = Alpha_Opaque;
  float alpha_cutoff = 0.5f;
  bool double_sided = false;

  // skybox
  SkyboxTexture *skybox_tex = nullptr;

  ModelMesh(ModelMesh &&o) = default;
  ModelMesh() = default;
};

struct ModelNode {
  glm::mat4 transform;
  std::vector<ModelMesh> meshes;
  std::vector<ModelNode> children;
};

}