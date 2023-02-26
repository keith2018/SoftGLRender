/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include <mutex>
#include <assimp/scene.h>

#include "base/buffer.h"
#include "model.h"
#include "config.h"
#include "config_panel.h"

namespace SoftGL {
namespace View {

class ModelLoader {
 public:
  explicit ModelLoader(Config &config, ConfigPanel &panel);

  bool LoadModel(const std::string &filepath);
  bool LoadSkybox(const std::string &filepath);

  inline DemoScene &GetScene() { return scene_; }

  inline size_t GetModelPrimitiveCnt() const {
    if (scene_.model) {
      return scene_.model->primitive_cnt;
    }
    return 0;
  }

  static void LoadCubeMesh(ModelVertexes &mesh);

 private:
  void LoadWorldAxis();
  void LoadLights();

  bool ProcessNode(const aiNode *ai_node, const aiScene *ai_scene, ModelNode &out_node, glm::mat4 &transform);
  bool ProcessMesh(const aiMesh *ai_mesh, const aiScene *ai_scene, ModelMesh &out_mesh);
  void ProcessMaterial(const aiMaterial *ai_material, aiTextureType texture_type, TexturedMaterial &material);

  static glm::mat4 ConvertMatrix(const aiMatrix4x4 &m);
  static BoundingBox ConvertBoundingBox(const aiAABB &aabb);

  void PreloadTextureFiles(const aiScene *scene, const std::string &res_dir);
  std::shared_ptr<Buffer<RGBA>> LoadTextureFile(const std::string &path);

 private:
  Config &config_;
  ConfigPanel &config_panel_;

  DemoScene scene_;
  std::unordered_map<std::string, std::shared_ptr<Model>> model_cache_;
  std::unordered_map<std::string, std::shared_ptr<Buffer<RGBA>>> texture_cache_;

  std::mutex load_mutex_;
  std::mutex tex_cache_mutex_;
};

}
}
