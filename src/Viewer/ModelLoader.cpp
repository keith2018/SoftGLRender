/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "ModelLoader.h"

#include <iostream>
#include <set>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>

#include "Base/ImageUtils.h"
#include "Base/ThreadPool.h"
#include "Base/StringUtils.h"
#include "Base/Logger.h"
#include "Cube.h"

namespace SoftGL {
namespace View {

ModelLoader::ModelLoader(Config &config) : config_(config) {
  loadWorldAxis();
  loadLights();
  loadFloor();
}

void ModelLoader::loadCubeMesh(ModelVertexes &mesh) {
  const float *cubeVertexes = Cube::getCubeVertexes();

  mesh.primitiveType = Primitive_TRIANGLE;
  mesh.primitiveCnt = 12;
  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 3; j++) {
      Vertex vertex{};
      vertex.a_position.x = cubeVertexes[i * 9 + j * 3 + 0];
      vertex.a_position.y = cubeVertexes[i * 9 + j * 3 + 1];
      vertex.a_position.z = cubeVertexes[i * 9 + j * 3 + 2];
      mesh.vertexes.push_back(vertex);
      mesh.indices.push_back(i * 3 + j);
    }
  }
  mesh.InitVertexes();
}

void ModelLoader::loadWorldAxis() {
  float axisY = -0.01f;
  int idx = 0;
  for (int i = -16; i <= 16; i++) {
    scene_.worldAxis.vertexes.push_back({glm::vec3(-3.2, axisY, 0.2f * (float) i)});
    scene_.worldAxis.vertexes.push_back({glm::vec3(3.2, axisY, 0.2f * (float) i)});
    scene_.worldAxis.indices.push_back(idx++);
    scene_.worldAxis.indices.push_back(idx++);

    scene_.worldAxis.vertexes.push_back({glm::vec3(0.2f * (float) i, axisY, -3.2)});
    scene_.worldAxis.vertexes.push_back({glm::vec3(0.2f * (float) i, axisY, 3.2)});
    scene_.worldAxis.indices.push_back(idx++);
    scene_.worldAxis.indices.push_back(idx++);
  }
  scene_.worldAxis.InitVertexes();

  scene_.worldAxis.primitiveType = Primitive_LINE;
  scene_.worldAxis.primitiveCnt = scene_.worldAxis.indices.size() / 2;
  scene_.worldAxis.material = std::make_shared<Material>();
  scene_.worldAxis.material->shadingModel = Shading_BaseColor;
  scene_.worldAxis.material->baseColor = glm::vec4(0.25f, 0.25f, 0.25f, 1.f);
  scene_.worldAxis.material->lineWidth = 1.f;
}

void ModelLoader::loadLights() {
  scene_.pointLight.primitiveType = Primitive_POINT;
  scene_.pointLight.primitiveCnt = 1;
  scene_.pointLight.vertexes.resize(scene_.pointLight.primitiveCnt);
  scene_.pointLight.indices.resize(scene_.pointLight.primitiveCnt);

  scene_.pointLight.vertexes[0] = {config_.pointLightPosition};
  scene_.pointLight.indices[0] = 0;
  scene_.pointLight.material = std::make_shared<Material>();
  scene_.pointLight.material->shadingModel = Shading_BaseColor;
  scene_.pointLight.material->baseColor = glm::vec4(config_.pointLightColor, 1.f);
  scene_.pointLight.material->pointSize = 10.f;
  scene_.pointLight.InitVertexes();
}

void ModelLoader::loadFloor() {
  float floorY = 0.01f;
  float floorSize = 2.0f;
  scene_.floor.vertexes.push_back({glm::vec3(-floorSize, floorY, floorSize), glm::vec2(0.f, 1.f),
                                   glm::vec3(0.f, 1.f, 0.f)});
  scene_.floor.vertexes.push_back({glm::vec3(-floorSize, floorY, -floorSize), glm::vec2(0.f, 0.f),
                                   glm::vec3(0.f, 1.f, 0.f)});
  scene_.floor.vertexes.push_back({glm::vec3(floorSize, floorY, -floorSize), glm::vec2(1.f, 0.f),
                                   glm::vec3(0.f, 1.f, 0.f)});
  scene_.floor.vertexes.push_back({glm::vec3(floorSize, floorY, floorSize), glm::vec2(1.f, 1.f),
                                   glm::vec3(0.f, 1.f, 0.f)});
  scene_.floor.indices.push_back(0);
  scene_.floor.indices.push_back(2);
  scene_.floor.indices.push_back(1);
  scene_.floor.indices.push_back(0);
  scene_.floor.indices.push_back(3);
  scene_.floor.indices.push_back(2);

  scene_.floor.primitiveType = Primitive_TRIANGLE;
  scene_.floor.primitiveCnt = 2;

  scene_.floor.material = std::make_shared<Material>();
  scene_.floor.material->shadingModel = Shading_BlinnPhong;
  scene_.floor.material->baseColor = glm::vec4(1.0f);
  scene_.floor.material->doubleSided = true;

  scene_.floor.aabb = BoundingBox(glm::vec3(-2, 0, -2), glm::vec3(2, 0, 2));
  scene_.floor.InitVertexes();
}

bool ModelLoader::loadSkybox(const std::string &filepath) {
  if (filepath.empty()) {
    return false;
  }
  if (scene_.skybox.primitiveCnt <= 0) {
    loadCubeMesh(scene_.skybox);
  }

  auto it = skyboxMaterialCache_.find(filepath);
  if (it != skyboxMaterialCache_.end()) {
    scene_.skybox.material = it->second;
    return true;
  }

  LOGD("load skybox, path: %s", filepath.c_str());
  auto material = std::make_shared<SkyboxMaterial>();
  material->shadingModel = Shading_Skybox;

  std::vector<std::shared_ptr<Buffer<RGBA>>> skyboxTex;
  if (StringUtils::endsWith(filepath, "/")) {
    skyboxTex.resize(6);

    ThreadPool pool(6);
    pool.pushTask([&](int thread_id) { skyboxTex[0] = loadTextureFile(filepath + "right.jpg"); });
    pool.pushTask([&](int thread_id) { skyboxTex[1] = loadTextureFile(filepath + "left.jpg"); });
    pool.pushTask([&](int thread_id) { skyboxTex[2] = loadTextureFile(filepath + "top.jpg"); });
    pool.pushTask([&](int thread_id) { skyboxTex[3] = loadTextureFile(filepath + "bottom.jpg"); });
    pool.pushTask([&](int thread_id) { skyboxTex[4] = loadTextureFile(filepath + "front.jpg"); });
    pool.pushTask([&](int thread_id) { skyboxTex[5] = loadTextureFile(filepath + "back.jpg"); });
    pool.waitTasksFinish();

    auto &texData = material->textureData[MaterialTexType_CUBE];
    texData.tag = filepath;
    texData.width = skyboxTex[0]->getWidth();
    texData.height = skyboxTex[0]->getHeight();
    texData.data = std::move(skyboxTex);
    texData.wrapModeU = Wrap_CLAMP_TO_EDGE;
    texData.wrapModeV = Wrap_CLAMP_TO_EDGE;
    texData.wrapModeW = Wrap_CLAMP_TO_EDGE;
  } else {
    skyboxTex.resize(1);
    skyboxTex[0] = loadTextureFile(filepath);

    auto &texData = material->textureData[MaterialTexType_EQUIRECTANGULAR];
    texData.tag = filepath;
    texData.width = skyboxTex[0]->getWidth();
    texData.height = skyboxTex[0]->getHeight();
    texData.data = std::move(skyboxTex);
    texData.wrapModeU = Wrap_CLAMP_TO_EDGE;
    texData.wrapModeV = Wrap_CLAMP_TO_EDGE;
    texData.wrapModeW = Wrap_CLAMP_TO_EDGE;
  }

  skyboxMaterialCache_[filepath] = material;
  scene_.skybox.material = material;
  return true;
}

bool ModelLoader::loadModel(const std::string &filepath) {
  std::lock_guard<std::mutex> lk(modelLoadMutex_);
  if (filepath.empty()) {
    return false;
  }

  auto it = modelCache_.find(filepath);
  if (it != modelCache_.end()) {
    scene_.model = it->second;
    return true;
  }

  modelCache_[filepath] = std::make_shared<Model>();
  scene_.model = modelCache_[filepath];

  LOGD("load model, path: %s", filepath.c_str());

  // load model
  Assimp::Importer importer;
  if (filepath.empty()) {
    LOGE("ModelLoader::loadModel, empty model file path.");
    return false;
  }
  const aiScene *scene = importer.ReadFile(filepath,
                                           aiProcess_Triangulate |
                                               aiProcess_CalcTangentSpace |
                                               aiProcess_FlipUVs |
                                               aiProcess_GenBoundingBoxes);
  if (!scene
      || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE
      || !scene->mRootNode) {
    LOGE("ModelLoader::loadModel, description: %s", importer.GetErrorString());
    return false;
  }
  scene_.model->resourcePath = filepath.substr(0, filepath.find_last_of('/'));

  // preload textures
  preloadTextureFiles(scene, scene_.model->resourcePath);

  auto currTransform = glm::mat4(1.f);
  if (!processNode(scene->mRootNode, scene, scene_.model->rootNode, currTransform)) {
    LOGE("ModelLoader::loadModel, process node failed.");
    return false;
  }

  // model center transform
  scene_.model->centeredTransform = adjustModelCenter(scene_.model->rootAABB);
  return true;
}

bool ModelLoader::processNode(const aiNode *ai_node,
                              const aiScene *ai_scene,
                              ModelNode &outNode,
                              glm::mat4 &transform) {
  if (!ai_node) {
    return false;
  }

  outNode.transform = convertMatrix(ai_node->mTransformation);
  auto currTransform = transform * outNode.transform;

  for (size_t i = 0; i < ai_node->mNumMeshes; i++) {
    const aiMesh *meshPtr = ai_scene->mMeshes[ai_node->mMeshes[i]];
    if (meshPtr) {
      ModelMesh mesh;
      if (processMesh(meshPtr, ai_scene, mesh)) {
        scene_.model->meshCnt++;
        scene_.model->primitiveCnt += mesh.primitiveCnt;
        scene_.model->vertexCnt += mesh.vertexes.size();

        // bounding box
        auto bounds = mesh.aabb.transform(currTransform);
        scene_.model->rootAABB.merge(bounds);

        outNode.meshes.push_back(std::move(mesh));
      }
    }
  }

  for (size_t i = 0; i < ai_node->mNumChildren; i++) {
    ModelNode childNode;
    if (processNode(ai_node->mChildren[i], ai_scene, childNode, currTransform)) {
      outNode.children.push_back(std::move(childNode));
    }
  }
  return true;
}

bool ModelLoader::processMesh(const aiMesh *ai_mesh, const aiScene *ai_scene, ModelMesh &outMesh) {
  std::vector<Vertex> vertexes;
  std::vector<int> indices;

  for (size_t i = 0; i < ai_mesh->mNumVertices; i++) {
    Vertex vertex{};
    if (ai_mesh->HasPositions()) {
      vertex.a_position.x = ai_mesh->mVertices[i].x;
      vertex.a_position.y = ai_mesh->mVertices[i].y;
      vertex.a_position.z = ai_mesh->mVertices[i].z;
    }
    if (ai_mesh->HasTextureCoords(0)) {
      vertex.a_texCoord.x = ai_mesh->mTextureCoords[0][i].x;
      vertex.a_texCoord.y = ai_mesh->mTextureCoords[0][i].y;
    } else {
      vertex.a_texCoord = glm::vec2(0.0f, 0.0f);
    }
    if (ai_mesh->HasNormals()) {
      vertex.a_normal.x = ai_mesh->mNormals[i].x;
      vertex.a_normal.y = ai_mesh->mNormals[i].y;
      vertex.a_normal.z = ai_mesh->mNormals[i].z;
    }
    if (ai_mesh->HasTangentsAndBitangents()) {
      vertex.a_tangent.x = ai_mesh->mTangents[i].x;
      vertex.a_tangent.y = ai_mesh->mTangents[i].y;
      vertex.a_tangent.z = ai_mesh->mTangents[i].z;
    }
    vertexes.push_back(vertex);
  }
  for (size_t i = 0; i < ai_mesh->mNumFaces; i++) {
    aiFace face = ai_mesh->mFaces[i];
    if (face.mNumIndices != 3) {
      LOGE("ModelLoader::processMesh, mesh not transformed to triangle mesh.");
      return false;
    }
    for (size_t j = 0; j < face.mNumIndices; ++j) {
      indices.push_back((int) (face.mIndices[j]));
    }
  }

  outMesh.material = std::make_shared<Material>();
  outMesh.material->baseColor = glm::vec4(1.f);
  if (ai_mesh->mMaterialIndex >= 0) {
    const aiMaterial *material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];

    // alpha mode
    outMesh.material->alphaMode = Alpha_Opaque;
    aiString alphaMode;
    if (material->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == aiReturn_SUCCESS) {
      // "MASK" not support
      if (aiString("BLEND") == alphaMode) {
        outMesh.material->alphaMode = Alpha_Blend;
      }
    }

    // double side
    outMesh.material->doubleSided = false;
    bool doubleSide;
    if (material->Get(AI_MATKEY_TWOSIDED, doubleSide) == aiReturn_SUCCESS) {
      outMesh.material->doubleSided = doubleSide;
    }

    // shading mode
    outMesh.material->shadingModel = Shading_BlinnPhong;  // default
    aiShadingMode shading_mode;
    if (material->Get(AI_MATKEY_SHADING_MODEL, shading_mode) == aiReturn_SUCCESS) {
      if (aiShadingMode_PBR_BRDF == shading_mode) {
        outMesh.material->shadingModel = Shading_PBR;
      }
    }

    for (int i = 0; i <= AI_TEXTURE_TYPE_MAX; i++) {
      processMaterial(material, static_cast<aiTextureType>(i), *outMesh.material);
    }
  }

  outMesh.primitiveType = Primitive_TRIANGLE;
  outMesh.primitiveCnt = ai_mesh->mNumFaces;
  outMesh.vertexes = std::move(vertexes);
  outMesh.indices = std::move(indices);
  outMesh.aabb = convertBoundingBox(ai_mesh->mAABB);
  outMesh.InitVertexes();

  return true;
}

void ModelLoader::processMaterial(const aiMaterial *ai_material,
                                  aiTextureType textureType,
                                  Material &material) {
  if (ai_material->GetTextureCount(textureType) <= 0) {
    return;
  }
  for (size_t i = 0; i < ai_material->GetTextureCount(textureType); i++) {
    aiTextureMapMode texMapMode[2];  // [u, v]
    aiString texPath;
    aiReturn retStatus = ai_material->GetTexture(textureType, i, &texPath,
                                                 nullptr, nullptr, nullptr, nullptr,
                                                 &texMapMode[0]);
    if (retStatus != aiReturn_SUCCESS || texPath.length == 0) {
      LOGW("load texture type=%d, index=%d failed with return value=%d", textureType, i, retStatus);
      continue;
    }
    std::string absolutePath = scene_.model->resourcePath + "/" + texPath.C_Str();
    MaterialTexType texType = MaterialTexType_NONE;
    switch (textureType) {
      case aiTextureType_BASE_COLOR:
      case aiTextureType_DIFFUSE:
        texType = MaterialTexType_ALBEDO;
        break;
      case aiTextureType_NORMALS:
        texType = MaterialTexType_NORMAL;
        break;
      case aiTextureType_EMISSIVE:
        texType = MaterialTexType_EMISSIVE;
        break;
      case aiTextureType_LIGHTMAP:
        texType = MaterialTexType_AMBIENT_OCCLUSION;
        break;
      case aiTextureType_UNKNOWN:
        texType = MaterialTexType_METAL_ROUGHNESS;
        break;
      default:
//        LOGW("texture type: %s not support", aiTextureTypeToString(textureType));
        continue;  // not support
    }

    auto buffer = loadTextureFile(absolutePath);
    if (buffer) {
      auto &texData = material.textureData[texType];
      texData.tag = absolutePath;
      texData.width = buffer->getWidth();
      texData.height = buffer->getHeight();
      texData.data = {buffer};
      texData.wrapModeU = convertTexWrapMode(texMapMode[0]);
      texData.wrapModeV = convertTexWrapMode(texMapMode[1]);
    } else {
      LOGE("load texture failed: %s, path: %s", Material::materialTexTypeStr(texType), absolutePath.c_str());
    }
  }
}

glm::mat4 ModelLoader::convertMatrix(const aiMatrix4x4 &m) {
  glm::mat4 ret;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ret[j][i] = m[i][j];
    }
  }
  return ret;
}

BoundingBox ModelLoader::convertBoundingBox(const aiAABB &aabb) {
  BoundingBox ret{};
  ret.min = glm::vec3(aabb.mMin.x, aabb.mMin.y, aabb.mMin.z);
  ret.max = glm::vec3(aabb.mMax.x, aabb.mMax.y, aabb.mMax.z);
  return ret;
}

WrapMode ModelLoader::convertTexWrapMode(const aiTextureMapMode &mode) {
  WrapMode retWrapMode;
  switch (mode) {
    case aiTextureMapMode_Wrap:
      retWrapMode = Wrap_REPEAT;
      break;
    case aiTextureMapMode_Clamp:
      retWrapMode = Wrap_CLAMP_TO_EDGE;
      break;
    case aiTextureMapMode_Mirror:
      retWrapMode = Wrap_MIRRORED_REPEAT;
      break;
    default:
      retWrapMode = Wrap_REPEAT;
      break;
  }

  return retWrapMode;
}

glm::mat4 ModelLoader::adjustModelCenter(BoundingBox &bounds) {
  glm::mat4 modelTransform(1.0f);
  glm::vec3 trans = (bounds.max + bounds.min) / -2.f;
  trans.y = -bounds.min.y;
  float bounds_len = glm::length(bounds.max - bounds.min);
  modelTransform = glm::scale(modelTransform, glm::vec3(3.f / bounds_len));
  modelTransform = glm::translate(modelTransform, trans);
  return modelTransform;
}

void ModelLoader::preloadTextureFiles(const aiScene *scene, const std::string &resDir) {
  std::set<std::string> texPaths;
  for (int materialIdx = 0; materialIdx < scene->mNumMaterials; materialIdx++) {
    aiMaterial *material = scene->mMaterials[materialIdx];
    for (int texType = aiTextureType_NONE; texType <= AI_TEXTURE_TYPE_MAX; texType++) {
      auto textureType = static_cast<aiTextureType>(texType);
      size_t texCnt = material->GetTextureCount(textureType);
      for (size_t i = 0; i < texCnt; i++) {
        aiString textPath;
        aiReturn retStatus = material->GetTexture(textureType, i, &textPath);
        if (retStatus != aiReturn_SUCCESS || textPath.length == 0) {
          continue;
        }
        texPaths.insert(resDir + "/" + textPath.C_Str());
      }
    }
  }
  if (texPaths.empty()) {
    return;
  }

  ThreadPool pool(std::min(texPaths.size(), (size_t) std::thread::hardware_concurrency()));
  for (auto &path : texPaths) {
    pool.pushTask([&](int thread_id) {
      loadTextureFile(path);
    });
  }
}

std::shared_ptr<Buffer<RGBA>> ModelLoader::loadTextureFile(const std::string &path) {
  texCacheMutex_.lock();
  if (textureDataCache_.find(path) != textureDataCache_.end()) {
    auto &buffer = textureDataCache_[path];
    texCacheMutex_.unlock();
    return buffer;
  }
  texCacheMutex_.unlock();

  LOGD("load texture, path: %s", path.c_str());

  auto buffer = ImageUtils::readImageRGBA(path);
  if (buffer == nullptr) {
    LOGD("load texture failed, path: %s", path.c_str());
    return nullptr;
  }

  texCacheMutex_.lock();
  textureDataCache_[path] = buffer;
  texCacheMutex_.unlock();

  return buffer;
}

}
}
