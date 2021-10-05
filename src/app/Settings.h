/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>

#include "ModelLoader.h"
#include "OrbitController.h"
#include "json11.hpp"

namespace SoftGL {
namespace App {

const std::string ASSETS_DIR = "../assets/";

class Settings {
 public:
  Settings() {
    LoadAssetConfig();

    model_name = model_paths_.begin()->first;
    model_path = model_paths_.begin()->second;

    skybox_name = skybox_paths_.begin()->first;
    skybox_path = skybox_paths_.begin()->second;
  }

  void LoadAssetConfig() {
    std::cout << "load assets in :" << ASSETS_DIR << std::endl;
    std::ifstream config(ASSETS_DIR + "assets.json");
    if (!config.is_open()) {
      std::cout << "open config file failed" << std::endl;
      return;
    }
    std::string config_str((std::istreambuf_iterator<char>(config)), std::istreambuf_iterator<char>());
    std::string err;
    const auto json = json11::Json::parse(config_str, err);
    for (auto &kv : json["model"].object_items()) {
      model_paths_[kv.first] = ASSETS_DIR + kv.second["path"].string_value();
    }
    for (auto &kv : json["skybox"].object_items()) {
      skybox_paths_[kv.first] = ASSETS_DIR + kv.second["path"].string_value();
    }

    if (model_paths_.empty()) {
      std::cout << "load models failed" << std::endl;
    }
  }

  void BindModelLoader(ModelLoader &loader) {
    model_loader_ = &loader;
  }

  void BindOrbitController(OrbitController &controller) {
    orbit_controller_ = &controller;
  }

  void ReloadModel(const std::string &name) {
    if (name != model_name) {
      model_name = name;
      model_path = model_paths_[model_name];
      if (model_loader_) {
        model_loader_->LoadModel(model_path);
      }
    }
  }

  void ReloadSkybox(const std::string &name) {
    if (name != skybox_name) {
      skybox_name = name;
      skybox_path = skybox_paths_[skybox_name];
      if (model_loader_) {
        model_loader_->LoadSkyBoxTex(skybox_path);
      }
    }
  }

  void ResetCamera() {
    if (orbit_controller_) {
      orbit_controller_->Reset();
    }
  }

  void Update() {
    light_position = 2.f * glm::vec3(glm::sin(light_position_angle),
                                     1.0f,
                                     glm::cos(light_position_angle));
    if (model_loader_) {
      triangle_count_ = model_loader_->GetFaceCount();
      model_loader_->SetPointLight(light_position, light_color);
    }
  }

  const std::unordered_map<std::string, std::string> &GetModelPaths() const {
    return model_paths_;
  }
  const std::unordered_map<std::string, std::string> &GetSkyboxPaths() const {
    return skybox_paths_;
  }

 public:
  std::string model_name;
  std::string model_path;
  std::string skybox_name;
  std::string skybox_path;

  size_t triangle_count_ = 0;
  bool wireframe = false;
  bool wireframe_show_clip = false;
  bool world_axis = true;
  bool show_skybox = false;
  bool cull_face = true;
  bool depth_test = true;
  bool early_z = true;
  int aa_type = AAType_NONE;

  glm::vec4 clear_color = {0.f, 0.f, 0.f, 0.f};
  glm::vec3 ambient_color = {0.5f, 0.5f, 0.5f};

  bool show_light = true;
  glm::vec3 light_color = {0.5f, 0.5f, 0.5f};
  glm::vec3 light_position{};
  float light_position_angle = glm::radians(0.f);

 private:
  ModelLoader *model_loader_ = nullptr;
  OrbitController *orbit_controller_ = nullptr;

  std::unordered_map<std::string, std::string> model_paths_;
  std::unordered_map<std::string, std::string> skybox_paths_;
};

}
}
