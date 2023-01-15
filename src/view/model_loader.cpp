/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "model_loader.h"

#include <iostream>
#include <set>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include <stb/stb_image_write.h>

#include "base/thread_pool.h"
#include "base/string_utils.h"
#include "base/logger.h"
#include "cube.h"

namespace SoftGL {
namespace View {

ModelLoader::ModelLoader(Config &config, ConfigPanel &panel)
    : config_(config), config_panel_(panel) {
  LoadWorldAxis();
  LoadLights();

  config_panel_.SetReloadModelFunc([&](const std::string &path) -> bool {
    return LoadModel(path);
  });
  config_panel_.SetReloadSkyboxFunc([&](const std::string &path) -> bool {
    return LoadSkybox(path);
  });
  config_panel_.SetUpdateLightFunc([&](glm::vec3 &position, glm::vec3 &color) -> void {
    scene_.point_light.vertexes[0].a_position = position;
    scene_.point_light.UpdateVertexData();
    scene_.point_light.material.base_color = glm::vec4(color, 1.f);
  });
}

void ModelLoader::LoadCubeMesh(VertexArray &mesh) {
  const float *cube_vertexes = Cube::GetCubeVertexes();

  mesh.primitive_type = Primitive_TRIANGLES;
  mesh.primitive_cnt = 12;
  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 3; j++) {
      Vertex vertex{};
      vertex.a_position.x = cube_vertexes[i * 9 + j * 3 + 0];
      vertex.a_position.y = cube_vertexes[i * 9 + j * 3 + 1];
      vertex.a_position.z = cube_vertexes[i * 9 + j * 3 + 2];
      mesh.vertexes.push_back(vertex);
      mesh.indices.push_back(i * 3 + j);
    }
  }
}

void ModelLoader::LoadWorldAxis() {
  float axis_y = -0.01f;
  int idx = 0;
  for (int i = -10; i <= 10; i++) {
    Vertex vertex_x0;
    Vertex vertex_x1;
    vertex_x0.a_position = glm::vec3(-2, axis_y, 0.2f * (float) i);
    vertex_x1.a_position = glm::vec3(2, axis_y, 0.2f * (float) i);
    scene_.world_axis.vertexes.push_back(vertex_x0);
    scene_.world_axis.vertexes.push_back(vertex_x1);
    scene_.world_axis.indices.push_back(idx++);
    scene_.world_axis.indices.push_back(idx++);

    Vertex vertex_y0;
    Vertex vertex_y1;
    vertex_y0.a_position = glm::vec3(0.2f * (float) i, axis_y, -2);
    vertex_y1.a_position = glm::vec3(0.2f * (float) i, axis_y, 2);
    scene_.world_axis.vertexes.push_back(vertex_y0);
    scene_.world_axis.vertexes.push_back(vertex_y1);
    scene_.world_axis.indices.push_back(idx++);
    scene_.world_axis.indices.push_back(idx++);
  }

  scene_.world_axis.primitive_type = Primitive_LINES;
  scene_.world_axis.primitive_cnt = scene_.world_axis.indices.size() / 2;
  scene_.world_axis.material.Reset();
  scene_.world_axis.material.shading = Shading_BaseColor;
  scene_.world_axis.material.base_color = glm::vec4(0.25f, 0.25f, 0.25f, 1.f);
  scene_.world_axis.line_width = 1.f;
}

void ModelLoader::LoadLights() {
  scene_.point_light.primitive_type = Primitive_POINTS;
  scene_.point_light.primitive_cnt = 1;
  scene_.point_light.vertexes.resize(scene_.point_light.primitive_cnt);
  scene_.point_light.indices.resize(scene_.point_light.primitive_cnt);

  Vertex vertex;
  vertex.a_position = config_.point_light_position;
  scene_.point_light.vertexes[0] = vertex;
  scene_.point_light.indices[0] = 0;
  scene_.point_light.material.Reset();
  scene_.point_light.material.shading = Shading_BaseColor;
  scene_.point_light.material.base_color = glm::vec4(config_.point_light_color, 1.f);
  scene_.point_light.point_size = 10.f;
}

bool ModelLoader::LoadSkybox(const std::string &filepath) {
  if (filepath.empty()) {
    return false;
  }
  if (scene_.skybox.primitive_cnt <= 0) {
    LoadCubeMesh(scene_.skybox);
  }

  LOGD("load skybox, path: %s", filepath.c_str());
  auto &material = scene_.skybox.material;
  material.Reset();
  material.shading = Shading_Skybox;

  std::vector<std::shared_ptr<BufferRGBA>> skybox_tex;
  if (StringUtils::EndsWith(filepath, "/")) {
    skybox_tex.resize(6);

    ThreadPool pool(6);
    pool.PushTask([&](int thread_id) { skybox_tex[0] = LoadTextureFile(filepath + "right.jpg"); });
    pool.PushTask([&](int thread_id) { skybox_tex[1] = LoadTextureFile(filepath + "left.jpg"); });
    pool.PushTask([&](int thread_id) { skybox_tex[2] = LoadTextureFile(filepath + "top.jpg"); });
    pool.PushTask([&](int thread_id) { skybox_tex[3] = LoadTextureFile(filepath + "bottom.jpg"); });
    pool.PushTask([&](int thread_id) { skybox_tex[4] = LoadTextureFile(filepath + "front.jpg"); });
    pool.PushTask([&](int thread_id) { skybox_tex[5] = LoadTextureFile(filepath + "back.jpg"); });
    pool.WaitTasksFinish();

    material.skybox_type = Skybox_Cube;
    material.texture_data[TextureUsage_CUBE] = std::move(skybox_tex);
  } else {
    material.skybox_type = Skybox_Equirectangular;
    skybox_tex.resize(1);
    skybox_tex[0] = LoadTextureFile(filepath);
    material.texture_data[TextureUsage_EQUIRECTANGULAR] = std::move(skybox_tex);
  }

  return true;
}

bool ModelLoader::LoadModel(const std::string &filepath) {
  std::lock_guard<std::mutex> lk(load_mutex_);
  if (filepath.empty()) {
    return false;
  }

  auto it = model_cache_.find(filepath);
  if (it != model_cache_.end()) {
    scene_.model = it->second;
    return true;
  }

  model_cache_[filepath] = std::make_shared<Model>();
  scene_.model = model_cache_[filepath];

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
  scene_.model->res_dir = filepath.substr(0, filepath.find_last_of('/'));

  // preload textures
  PreloadTextureFiles(scene, scene_.model->res_dir);

  auto curr_transform = glm::mat4(1.f);
  if (!ProcessNode(scene->mRootNode, scene, scene_.model->root_node, curr_transform)) {
    LOGE("ModelLoader::loadModel, process node failed.");
    return false;
  }

  return true;
}

bool ModelLoader::ProcessNode(const aiNode *ai_node,
                              const aiScene *ai_scene,
                              ModelNode &out_node,
                              glm::mat4 &transform) {
  if (!ai_node) {
    return false;
  }

  out_node.transform = ConvertMatrix(ai_node->mTransformation);
  auto curr_transform = transform * out_node.transform;

  for (size_t i = 0; i < ai_node->mNumMeshes; i++) {
    const aiMesh *meshPtr = ai_scene->mMeshes[ai_node->mMeshes[i]];
    if (meshPtr) {
      ModelMesh mesh;
      if (ProcessMesh(meshPtr, ai_scene, mesh)) {
        scene_.model->mesh_cnt++;
        scene_.model->primitive_cnt += mesh.primitive_cnt;
        scene_.model->vertex_cnt += mesh.vertexes.size();

        // bounding box
        auto bounds = mesh.aabb.Transform(curr_transform);
        scene_.model->root_aabb.Merge(bounds);

        out_node.meshes.push_back(std::move(mesh));
      }
    }
  }

  for (size_t i = 0; i < ai_node->mNumChildren; i++) {
    ModelNode child_node;
    if (ProcessNode(ai_node->mChildren[i], ai_scene, child_node, curr_transform)) {
      out_node.children.push_back(std::move(child_node));
    }
  }
  return true;
}

bool ModelLoader::ProcessMesh(const aiMesh *ai_mesh, const aiScene *ai_scene, ModelMesh &out_mesh) {
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

  out_mesh.material_textured.Reset();
  if (ai_mesh->mMaterialIndex >= 0) {
    const aiMaterial *material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
    aiString alphaMode;
    material->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode);
    // "MASK" not support
    if (aiString("BLEND") == alphaMode) {
      out_mesh.material_textured.alpha_mode = Alpha_Blend;
    } else {
      out_mesh.material_textured.alpha_mode = Alpha_Opaque;
    }
    material->Get(AI_MATKEY_TWOSIDED, out_mesh.material_textured.double_sided);

    aiShadingMode shading_mode;
    material->Get(AI_MATKEY_SHADING_MODEL, shading_mode);
    out_mesh.material_textured.shading = Shading_BlinnPhong;  // default
    if (aiShadingMode_PBR_BRDF == shading_mode) {
      out_mesh.material_textured.shading = Shading_PBR;
    }

    for (int i = 0; i <= AI_TEXTURE_TYPE_MAX; i++) {
      ProcessMaterial(material, static_cast<aiTextureType>(i), out_mesh.material_textured);
    }
  }

  out_mesh.material_wireframe.Reset();
  out_mesh.material_wireframe.shading = Shading_BaseColor;
  out_mesh.material_wireframe.base_color = glm::vec4(1.f);

  out_mesh.primitive_type = Primitive_TRIANGLES;
  out_mesh.primitive_cnt = ai_mesh->mNumFaces;
  out_mesh.vertexes = std::move(vertexes);
  out_mesh.indices = std::move(indices);
  out_mesh.aabb = ConvertBoundingBox(ai_mesh->mAABB);

  return true;
}

void ModelLoader::ProcessMaterial(const aiMaterial *ai_material,
                                  aiTextureType texture_type,
                                  TexturedMaterial &material) {
  if (ai_material->GetTextureCount(texture_type) <= 0) {
    return;
  }
  for (size_t i = 0; i < ai_material->GetTextureCount(texture_type); i++) {
    aiString text_path;
    aiReturn retStatus = ai_material->GetTexture(texture_type, i, &text_path);
    if (retStatus != aiReturn_SUCCESS || text_path.length == 0) {
      LOGW("load texture type=%d, index=%d failed with return value=%d", texture_type, i, retStatus);
      continue;
    }
    std::string absolutePath = scene_.model->res_dir + "/" + text_path.C_Str();
    TextureUsage usage = TextureUsage_NONE;
    switch (texture_type) {
      case aiTextureType_BASE_COLOR:
      case aiTextureType_DIFFUSE:
        usage = TextureUsage_ALBEDO;
        break;
      case aiTextureType_NORMALS:
        usage = TextureUsage_NORMAL;
        break;
      case aiTextureType_EMISSIVE:
        usage = TextureUsage_EMISSIVE;
        break;
      case aiTextureType_LIGHTMAP:
        usage = TextureUsage_AMBIENT_OCCLUSION;
        break;
      case aiTextureType_UNKNOWN:
        usage = TextureUsage_METAL_ROUGHNESS;
        break;
      default:
//        LOGW("texture type: %s not support", aiTextureTypeToString(texture_type));
        continue;  // not support
    }

    auto buffer = LoadTextureFile(absolutePath);
    if (buffer) {
      material.texture_data[usage] = {buffer};
    } else {
      LOGE("load texture failed: %s, path: %s", Material::TextureUsageStr(usage), absolutePath.c_str());
    }
  }
}

glm::mat4 ModelLoader::ConvertMatrix(const aiMatrix4x4 &m) {
  glm::mat4 ret;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ret[j][i] = m[i][j];
    }
  }
  return ret;
}

BoundingBox ModelLoader::ConvertBoundingBox(const aiAABB &aabb) {
  BoundingBox ret{};
  ret.min = glm::vec3(aabb.mMin.x, aabb.mMin.y, aabb.mMin.z);
  ret.max = glm::vec3(aabb.mMax.x, aabb.mMax.y, aabb.mMax.z);
  return ret;
}

void ModelLoader::PreloadTextureFiles(const aiScene *scene, const std::string &res_dir) {
  std::set<std::string> tex_paths;
  for (int material_idx = 0; material_idx < scene->mNumMaterials; material_idx++) {
    aiMaterial *material = scene->mMaterials[material_idx];
    for (int tex_type = aiTextureType_NONE; tex_type <= AI_TEXTURE_TYPE_MAX; tex_type++) {
      auto texture_type = static_cast<aiTextureType>(tex_type);
      size_t tex_cnt = material->GetTextureCount(texture_type);
      for (size_t i = 0; i < tex_cnt; i++) {
        aiString text_path;
        aiReturn retStatus = material->GetTexture(texture_type, i, &text_path);
        if (retStatus != aiReturn_SUCCESS || text_path.length == 0) {
          continue;
        }
        tex_paths.insert(res_dir + "/" + text_path.C_Str());
      }
    }
  }
  if (tex_paths.empty()) {
    return;
  }

  ThreadPool pool(std::min(tex_paths.size(), (size_t) std::thread::hardware_concurrency()));
  for (auto &path : tex_paths) {
    pool.PushTask([&](int thread_id) {
      LoadTextureFile(path);
    });
  }
}

std::shared_ptr<BufferRGBA> ModelLoader::LoadTextureFile(const std::string &path) {
  tex_cache_mutex_.lock();
  if (texture_cache_.find(path) != texture_cache_.end()) {
    auto &buffer = texture_cache_[path];
    tex_cache_mutex_.unlock();
    return buffer;
  }
  tex_cache_mutex_.unlock();

  LOGD("load texture, path: %s", path.c_str());

  int iw = 0, ih = 0, n = 0;
  unsigned char *data = stbi_load(path.c_str(), &iw, &ih, &n, STBI_default);
  if (data == nullptr) {
    LOGD("load texture failed, path: %s", path.c_str());
    return nullptr;
  }
  auto buffer = BufferRGBA::MakeDefault();
  buffer->Create(iw, ih);

  // convert to rgba
  for (size_t y = 0; y < ih; y++) {
    for (size_t x = 0; x < iw; x++) {
      auto &to = *buffer->Get(x, y);
      size_t idx = x + y * iw;

      switch (n) {
        case STBI_grey: {
          to.r = data[idx];
          to.g = to.b = to.r;
          to.a = 255;
          break;
        }
        case STBI_grey_alpha: {
          to.r = data[idx * 2 + 0];
          to.g = to.b = to.r;
          to.a = data[idx * 2 + 1];
          break;
        }
        case STBI_rgb: {
          to.r = data[idx * 3 + 0];
          to.g = data[idx * 3 + 1];
          to.b = data[idx * 3 + 2];
          to.a = 255;
          break;
        }
        case STBI_rgb_alpha: {
          to.r = data[idx * 4 + 0];
          to.g = data[idx * 4 + 1];
          to.b = data[idx * 4 + 2];
          to.a = data[idx * 4 + 3];
          break;
        }
        default:
          break;
      }
    }
  }

  stbi_image_free(data);

  tex_cache_mutex_.lock();
  texture_cache_[path] = buffer;
  tex_cache_mutex_.unlock();

  return buffer;
}

}
}