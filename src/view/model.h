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
  PrimitiveType primitiveType;
  size_t primitiveCnt = 0;
  std::vector<Vertex> vertexes;
  std::vector<int32_t> indices;
  std::shared_ptr<VertexArrayObject> vao = nullptr;

  void UpdateVertexes() const {
    if (vao) {
      vao->updateVertexData(vertexesBuffer, vertexesBufferLength);
    }
  };

  void InitVertexes() {
    vertexesDesc.resize(4);
    vertexesDesc[0] = {3, sizeof(Vertex), offsetof(Vertex, a_position)};
    vertexesDesc[1] = {2, sizeof(Vertex), offsetof(Vertex, a_texCoord)};
    vertexesDesc[2] = {3, sizeof(Vertex), offsetof(Vertex, a_normal)};
    vertexesDesc[3] = {3, sizeof(Vertex), offsetof(Vertex, a_tangent)};

    vertexesBuffer = vertexes.empty() ? nullptr : (uint8_t *) &vertexes[0];
    vertexesBufferLength = vertexes.size() * sizeof(Vertex);

    indicesBuffer = indices.empty() ? nullptr : &indices[0];
    indicesBufferLength = indices.size() * sizeof(int32_t);
  }
};

struct ModelPoints : ModelVertexes {
  float pointSize;
  BaseColorMaterial material;
};

struct ModelLines : ModelVertexes {
  float lineWidth;
  BaseColorMaterial material;
};

struct ModelMesh : ModelVertexes {
  BoundingBox aabb;
  BaseColorMaterial materialBaseColor;
  TexturedMaterial materialTextured;
};

struct ModelSkybox : ModelVertexes {
  std::unordered_map<std::string, SkyboxMaterial> materialCache;
  SkyboxMaterial *material = nullptr;
};

struct ModelNode {
  glm::mat4 transform;
  std::vector<ModelMesh> meshes;
  std::vector<ModelNode> children;
};

struct Model {
  std::string resDir;

  ModelNode rootNode;
  BoundingBox rootAABB;

  size_t meshCnt = 0;
  size_t primitiveCnt = 0;
  size_t vertexCnt = 0;

  glm::mat4 centeredTransform;
};

struct DemoScene {
  std::shared_ptr<Model> model;
  ModelMesh floor;
  ModelLines worldAxis;
  ModelPoints pointLight;
  ModelSkybox skybox;

  void resetModelTextures() {
    std::function<void(ModelNode &node)> resetNodeFunc = [&](ModelNode &node) -> void {
      for (auto &mesh : node.meshes) {
        mesh.materialTextured.resetRuntimeStates();
      }
      for (auto &childNode : node.children) {
        resetNodeFunc(childNode);
      }
    };
    resetNodeFunc(model->rootNode);
  }

  void resetAllStates() {
    std::function<void(ModelNode &node)> resetNodeFunc = [&](ModelNode &node) -> void {
      for (auto &mesh : node.meshes) {
        mesh.vao = nullptr;
        mesh.materialBaseColor.resetRuntimeStates();
        mesh.materialTextured.resetRuntimeStates();
      }
      for (auto &childNode : node.children) {
        resetNodeFunc(childNode);
      }
    };
    resetNodeFunc(model->rootNode);

    floor.vao = nullptr;
    floor.materialBaseColor.resetRuntimeStates();
    floor.materialTextured.resetRuntimeStates();

    worldAxis.vao = nullptr;
    worldAxis.material.resetRuntimeStates();

    pointLight.vao = nullptr;
    pointLight.material.resetRuntimeStates();

    skybox.vao = nullptr;
    for (auto &kv : skybox.materialCache) {
      kv.second.resetRuntimeStates();
    }
  }
};

}
}
