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

#include "environment.h"
#include "base/thread_pool.h"
#include "base/string_utils.h"
#include "base/logger.h"

namespace SoftGL {
namespace View {

float skyboxVertices[] = {
    // positions
    -1.0f, 1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,

    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f
};

std::unordered_map<std::string, std::shared_ptr<Buffer<glm::u8vec4>>> ModelLoader::texture_cache_;

void SkyboxTextureIBL::InitIBL() {
  if (ibl_running) {
    return;
  }
  ibl_running = true;

  if (!cube_ready && equirectangular_ready) {
    LOGD("convert equirectangular to cube map ...");
    Environment::ConvertEquirectangular(equirectangular, cube);
    cube_ready = true;
    LOGD("convert equirectangular to cube map done.");
  }

  std::thread irradiance_thread([&]() {
    if (!irradiance_ready && cube_ready) {
      LOGD("generate irradiance map ...");
      Environment::GenerateIrradianceMap(cube, irradiance);
      irradiance_ready = true;
      LOGD("generate irradiance map done.");
    }
  });
  irradiance_thread.detach();

  std::thread prefilter_thread([&]() {
    if (!prefilter_ready && cube_ready) {
      LOGD("generate prefilter map ...");
      Environment::GeneratePrefilterMap(cube, prefilter);
      prefilter_ready = true;
      LOGD("generate prefilter map done.");
    }
  });
  prefilter_thread.detach();
}

std::shared_ptr<ModelMesh> ModelLoader::skybox_mash_ = nullptr;
SkyboxTextureIBL *ModelLoader::curr_skybox_tex_ = nullptr;

ModelLoader::ModelLoader() {
  // world axis
  LoadWorldAxis();

  // lights
  LoadLights();
}

bool ModelLoader::LoadModel(const std::string &filepath) {
  std::lock_guard<std::mutex> lk(model_load_mutex_);
  if (filepath.empty()) {
    return false;
  }

  auto it = model_cache_.find(filepath);
  if (it != model_cache_.end()) {
    curr_model_ = &it->second;
    return true;
  }

  model_cache_[filepath] = ModelContainer();
  curr_model_ = &model_cache_[filepath];

  LOGD("load model, path: %s", filepath.c_str());

  // load model
  Assimp::Importer importer;
  if (filepath.empty()) {
    LOGE("ModelObj::loadModel, empty model file path.");
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
    LOGE("ModelObj::loadModel, description: %s", importer.GetErrorString());
    return false;
  }
  curr_model_->model_file_dir = filepath.substr(0, filepath.find_last_of(FILE_SEPARATOR));

  // preload textures
  PreloadSceneTextureFiles(scene, curr_model_->model_file_dir);

  auto curr_transform = glm::mat4(1.f);
  if (!ProcessNode(scene->mRootNode, scene, curr_model_->root_node, curr_transform)) {
    LOGE("ModelObj::loadModel, process node failed.");
    return false;
  }

  return true;
}

void ModelLoader::LoadSkyBoxTex(const std::string &filepath) {
  if (filepath.empty()) {
    return;
  }

  auto it = skybox_tex_cache_.find(filepath);
  if (it != skybox_tex_cache_.end()) {
    curr_skybox_tex_ = &it->second;
    ReloadSkyboxMesh();
    return;
  }

  skybox_tex_cache_[filepath] = SkyboxTextureIBL();
  curr_skybox_tex_ = &skybox_tex_cache_[filepath];

  LOGD("load skybox, path: %s", filepath.c_str());

  if (StringUtils::EndsWith(filepath, "/")) {
    curr_skybox_tex_->type = Skybox_Cube;
    curr_skybox_tex_->cube[0].type = TextureType_CUBE_MAP_POSITIVE_X;
    curr_skybox_tex_->cube[1].type = TextureType_CUBE_MAP_NEGATIVE_X;
    curr_skybox_tex_->cube[2].type = TextureType_CUBE_MAP_POSITIVE_Y;
    curr_skybox_tex_->cube[3].type = TextureType_CUBE_MAP_NEGATIVE_Y;
    curr_skybox_tex_->cube[4].type = TextureType_CUBE_MAP_POSITIVE_Z;
    curr_skybox_tex_->cube[5].type = TextureType_CUBE_MAP_NEGATIVE_Z;

    ThreadPool pool(6);
    pool.PushTask([&](int thread_id) { LoadTextureFile(curr_skybox_tex_->cube[0], (filepath + "right.jpg").c_str()); });
    pool.PushTask([&](int thread_id) { LoadTextureFile(curr_skybox_tex_->cube[1], (filepath + "left.jpg").c_str()); });
    pool.PushTask([&](int thread_id) { LoadTextureFile(curr_skybox_tex_->cube[2], (filepath + "top.jpg").c_str()); });
    pool.PushTask([&](int thread_id) { LoadTextureFile(curr_skybox_tex_->cube[3], (filepath + "bottom.jpg").c_str()); });
    pool.PushTask([&](int thread_id) { LoadTextureFile(curr_skybox_tex_->cube[4], (filepath + "front.jpg").c_str()); });
    pool.PushTask([&](int thread_id) { LoadTextureFile(curr_skybox_tex_->cube[5], (filepath + "back.jpg").c_str()); });
    pool.WaitTasksFinish();
    curr_skybox_tex_->cube_ready = true;
  } else {
    curr_skybox_tex_->type = Skybox_Equirectangular;
    curr_skybox_tex_->equirectangular.type = TextureType_EQUIRECTANGULAR;
    LoadTextureFile(curr_skybox_tex_->equirectangular, filepath.c_str());
    curr_skybox_tex_->equirectangular_ready = true;
  }

  // reset skybox mesh
  ReloadSkyboxMesh();
}

void ModelLoader::ReloadSkyboxMesh() {
  skybox_mash_ = std::make_shared<ModelMesh>();
  skybox_mash_->shading_type = ShadingType_SKYBOX;
  skybox_mash_->primitive_cnt = 12;
  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 3; j++) {
      Vertex vertex{};
      vertex.a_position.x = skyboxVertices[i * 9 + j * 3 + 0];
      vertex.a_position.y = skyboxVertices[i * 9 + j * 3 + 1];
      vertex.a_position.z = skyboxVertices[i * 9 + j * 3 + 2];
      skybox_mash_->vertexes.push_back(vertex);
      skybox_mash_->indices.push_back(i * 3 + j);
    }
  }
  skybox_mash_->skybox_tex = curr_skybox_tex_;
}

void ModelLoader::LoadWorldAxis() {
  if (world_axis_.primitive_cnt > 0) {
    return;
  }

  float axis_y = -0.01f;
  int idx = 0;
  for (int i = -10; i <= 10; i++) {
    Vertex vertex_x0{};
    Vertex vertex_x1{};
    vertex_x0.a_position = glm::vec3(-2, axis_y, 0.2f * (float) i);
    vertex_x1.a_position = glm::vec3(2, axis_y, 0.2f * (float) i);
    world_axis_.vertexes.push_back(vertex_x0);
    world_axis_.vertexes.push_back(vertex_x1);
    world_axis_.indices.push_back(idx++);
    world_axis_.indices.push_back(idx++);

    Vertex vertex_y0{};
    Vertex vertex_y1{};
    vertex_y0.a_position = glm::vec3(0.2f * (float) i, axis_y, -2);
    vertex_y1.a_position = glm::vec3(0.2f * (float) i, axis_y, 2);
    world_axis_.vertexes.push_back(vertex_y0);
    world_axis_.vertexes.push_back(vertex_y1);
    world_axis_.indices.push_back(idx++);
    world_axis_.indices.push_back(idx++);
  }
  world_axis_.primitive_cnt = world_axis_.indices.size() / 2;
  world_axis_.line_color = glm::vec4(0.25f, 0.25f, 0.25f, 1.f);
  world_axis_.line_width = 1.f;
}

void ModelLoader::LoadLights() {
  lights_.primitive_cnt = 1;
  lights_.vertexes.resize(lights_.primitive_cnt);
  lights_.indices.resize(lights_.primitive_cnt);

  Vertex vertex{};
  vertex.a_position = point_light_position_;
  lights_.vertexes[0] = vertex;
  lights_.indices[0] = 0;
  lights_.point_color = glm::vec4(point_light_color_, 1.f);
  lights_.point_size = 10.f;
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
        curr_model_->mesh_count++;
        curr_model_->face_count += mesh.primitive_cnt;
        curr_model_->vertex_count += mesh.vertexes.size();
        mesh.idx = curr_model_->mesh_count - 1;

        // bounding box
        auto bounds = mesh.bounding_box.Transform(curr_transform);
        curr_model_->root_bounding_box.Merge(bounds);

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
  std::unordered_map<TextureType, Texture, EnumClassHash> textures;
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
      LOGE("ModelObj::processMesh, mesh not transformed to triangle mesh.");
      return false;
    }
    for (size_t j = 0; j < face.mNumIndices; ++j) {
      indices.push_back((int) (face.mIndices[j]));
    }
  }

  if (ai_mesh->mMaterialIndex >= 0) {
    const aiMaterial *material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
    aiString alphaMode;
    material->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode);
    if (aiString("MASK") == alphaMode) {
      out_mesh.alpha_mode = Alpha_Mask;
    } else if (aiString("BLEND") == alphaMode) {
      out_mesh.alpha_mode = Alpha_Blend;
    } else {
      out_mesh.alpha_mode = Alpha_Opaque;
    }
    material->Get(AI_MATKEY_GLTF_ALPHACUTOFF, out_mesh.alpha_cutoff);
    material->Get(AI_MATKEY_TWOSIDED, out_mesh.double_sided);

    aiShadingMode shading_mode;
    material->Get(AI_MATKEY_SHADING_MODEL, shading_mode);
    if (aiShadingMode_Blinn == shading_mode) {
      out_mesh.shading_type = ShadingType_BLINN_PHONG;
    } else if (aiShadingMode_PBR_BRDF == shading_mode) {
      out_mesh.shading_type = ShadingType_PBR_BRDF;
    } else {
      out_mesh.shading_type = ShadingType_BLINN_PHONG;  // default
    }

    for (int i = 0; i <= AI_TEXTURE_TYPE_MAX; i++) {
      ProcessMaterial(material, static_cast<aiTextureType>(i), textures);
    }
  }

  out_mesh.primitive_cnt = ai_mesh->mNumFaces;
  out_mesh.vertexes = std::move(vertexes);
  out_mesh.indices = std::move(indices);
  out_mesh.textures = std::move(textures);
  out_mesh.bounding_box = ConvertBoundingBox(ai_mesh->mAABB);

  return true;
}

bool ModelLoader::ProcessMaterial(const aiMaterial *ai_material,
                                  aiTextureType texture_type,
                                  std::unordered_map<TextureType, Texture, EnumClassHash> &textures) {
  if (ai_material->GetTextureCount(texture_type) <= 0) {
    return true;
  }
  for (size_t i = 0; i < ai_material->GetTextureCount(texture_type); i++) {
    aiString text_path;
    aiReturn retStatus = ai_material->GetTexture(texture_type, i, &text_path);
    if (retStatus != aiReturn_SUCCESS || text_path.length == 0) {
      LOGW("load texture type=%d, index=%d failed with return value=%d", texture_type, i, retStatus);
      continue;
    }
    std::string absolutePath = curr_model_->model_file_dir + FILE_SEPARATOR + text_path.C_Str();
    TextureType type;
    switch (texture_type) {
      case aiTextureType_DIFFUSE: 
        type = TextureType_DIFFUSE;
        break;
      case aiTextureType_NORMALS: 
        type = TextureType_NORMALS;
        break;
      case aiTextureType_EMISSIVE: 
        type = TextureType_EMISSIVE;
        break;
      case aiTextureType_BASE_COLOR: 
        type = TextureType_PBR_ALBEDO;
        break;
      case aiTextureType_UNKNOWN: 
        type = TextureType_PBR_METAL_ROUGHNESS;
        break;
      case aiTextureType_LIGHTMAP: 
        type = TextureType_PBR_AMBIENT_OCCLUSION;
        break;
      default:
//        LOGW("texture type: %s not support", aiTextureTypeToString(texture_type));
        return true;  // not support
    }

    Texture text;
    text.type = type;
    bool load_ok = LoadTextureFile(text, absolutePath.c_str());
    if (load_ok) {
      textures[text.type] = text;
    } else {
      LOGE("load texture failed: %s, path: %s", Texture::TextureTypeString(type), absolutePath.c_str());
    }
  }
  return true;
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

void ModelLoader::PreloadSceneTextureFiles(const aiScene *scene, const std::string &res_dir) {
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
        tex_paths.insert(res_dir + FILE_SEPARATOR + text_path.C_Str());
      }
    }
  }
  if (tex_paths.empty()) {
    return;
  }

  ThreadPool pool(std::min(tex_paths.size(), (size_t) std::thread::hardware_concurrency()));
  for (auto &path : tex_paths) {
    pool.PushTask([&](int thread_id) {
      Texture tex;
      LoadTextureFile(tex, path.c_str());
    });
  }
}

bool ModelLoader::LoadTextureFile(SoftGL::Texture &tex, const char *path) {
  if (texture_cache_.find(path) != texture_cache_.end()) {
    tex.buffer = texture_cache_[path];
    return true;
  }

  LOGD("load texture, path: %s", path);

  int iw = 0, ih = 0, n = 0;
  unsigned char *data = stbi_load(path, &iw, &ih, &n, STBI_default);
  if (data == nullptr) {
    return false;
  }
  tex.buffer = Texture::CreateBufferDefault();
  tex.buffer->Create(iw, ih);
  auto &buffer = tex.buffer;

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
        default:break;
      }
    }
  }

  stbi_image_free(data);
  texture_cache_[path] = tex.buffer;

  return true;
}

}
}