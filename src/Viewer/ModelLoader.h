/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include <mutex>
#include <assimp/scene.h>

#include "Base/Buffer.h"
#include "Model.h"
#include "Config.h"
#include "ConfigPanel.h"

namespace SoftGL {
namespace View {

class ModelLoader {
 public:
  explicit ModelLoader(Config &config, ConfigPanel &panel);

  bool loadModel(const std::string &filepath);
  bool loadSkybox(const std::string &filepath);

  inline DemoScene &getScene() { return scene_; }

  inline size_t getModelPrimitiveCnt() const {
    if (scene_.model) {
      return scene_.model->primitiveCnt;
    }
    return 0;
  }

  static void loadCubeMesh(ModelVertexes &mesh);

 private:
  void loadWorldAxis();
  void loadLights();
  void loadFloor();

  bool processNode(const aiNode *ai_node, const aiScene *ai_scene, ModelNode &outNode, glm::mat4 &transform);
  bool processMesh(const aiMesh *ai_mesh, const aiScene *ai_scene, ModelMesh &outMesh);
  void processMaterial(const aiMaterial *ai_material, aiTextureType textureType, TexturedMaterial &material);

  static glm::mat4 convertMatrix(const aiMatrix4x4 &m);
  static BoundingBox convertBoundingBox(const aiAABB &aabb);
  static glm::mat4 adjustModelCenter(BoundingBox &bounds);

  void preloadTextureFiles(const aiScene *scene, const std::string &resDir);
  std::shared_ptr<Buffer<RGBA>> loadTextureFile(const std::string &path);

 private:
  Config &config_;
  ConfigPanel &configPanel_;

  DemoScene scene_;
  std::unordered_map<std::string, std::shared_ptr<Model>> modelCache_;
  std::unordered_map<std::string, std::shared_ptr<Buffer<RGBA>>> textureCache_;

  std::mutex loadMutex_;
  std::mutex texCacheMutex_;
};

}
}
