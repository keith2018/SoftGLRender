/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include <assimp/scene.h>

#include "base/model.h"
#include "base/texture.h"

namespace SoftGL {
namespace View {

enum SkyboxTextureType {
  Skybox_Cube,
  Skybox_Equirectangular,
};

struct SkyboxTexture {
  SkyboxTextureType type;
  Texture cube[6];  // +x, -x, +y, -y, +z, -z
  Texture equirectangular;

  // IBL
  Texture irradiance[6];
  Texture prefilter[6];

  bool cube_ready = false;
  bool equirectangular_ready = false;
  bool irradiance_ready = false;
  bool prefilter_ready = false;
  bool ibl_running = false;

  void InitIBL();
};

struct ModelContainer {
  std::string model_file_dir;
  size_t mesh_count = 0;
  size_t face_count = 0;
  size_t vertex_count = 0;
  ModelNode root_node;
  BoundingBox root_bounding_box;
};

class ModelLoader {
 public:
  ModelLoader();

  bool LoadModel(const std::string &filepath);

  void LoadSkyBoxTex(const std::string &filepath);

  inline ModelNode *GetRootNode() {
    return curr_model_ ? &curr_model_->root_node : nullptr;
  }

  inline BoundingBox *GetRootBoundingBox() {
    return curr_model_ ? &curr_model_->root_bounding_box : nullptr;
  }

  inline size_t GetMeshCount() const {
    return curr_model_ ? curr_model_->mesh_count : 0;;
  }

  inline size_t GetFaceCount() const {
    return curr_model_ ? curr_model_->face_count : 0;
  }

  inline size_t GetVertexCount() const {
    return curr_model_ ? curr_model_->vertex_count : 0;
  }

  inline static ModelMesh &GetSkyBoxMesh() {
    LoadSkyboxMesh();
    return skybox_mash_;
  }

  inline SkyboxTexture *GetSkyBoxTexture() {
    return curr_skybox_tex_;
  }

  inline ModelLines &GetWorldAxisLines() {
    return world_axis_;
  }

  inline ModelPoints &GetLights() {
    return lights_;
  }

  void SetPointLight(const glm::vec3 &pos, const glm::vec3 &color) {
    point_light_position_ = pos;
    point_light_color_ = color;

    // reload
    LoadLights();
  }

 private:
  bool ProcessNode(const aiNode *ai_node, const aiScene *ai_scene, ModelNode &out_node, glm::mat4 &transform);
  bool ProcessMesh(const aiMesh *ai_mesh, const aiScene *ai_scene, ModelMesh &out_mesh);
  bool ProcessMaterial(const aiMaterial *ai_material,
                       aiTextureType texture_type,
                       std::unordered_map<TextureType, Texture, EnumClassHash> &textures);
  static glm::mat4 ConvertMatrix(const aiMatrix4x4 &m);
  static BoundingBox ConvertBoundingBox(const aiAABB &aabb);

  void LoadWorldAxis();
  void LoadLights();
  static void LoadSkyboxMesh();

  static void PreloadSceneTextureFiles(const aiScene *scene, const std::string &res_dir);
  static bool LoadTextureFile(Texture &tex, const char *path);

 private:
  // model
  ModelContainer *curr_model_ = nullptr;
  std::unordered_map<std::string, ModelContainer> model_cache_;

  // skybox
  static ModelMesh skybox_mash_;
  SkyboxTexture *curr_skybox_tex_ = nullptr;
  std::unordered_map<std::string, SkyboxTexture> skybox_tex_cache_;

  // world axis
  ModelLines world_axis_;

  // light
  ModelPoints lights_;
  glm::vec3 point_light_position_;
  glm::vec3 point_light_color_;

 private:
  std::mutex model_load_mutex_;
  static std::unordered_map<std::string, std::shared_ptr<Buffer<glm::u8vec4>>> texture_cache_;
};

}
}
