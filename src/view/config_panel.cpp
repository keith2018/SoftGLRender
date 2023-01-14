/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "config_panel.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "json11.hpp"
#include "base/logger.h"
#include "base/file_utils.h"

namespace SoftGL {
namespace View {

bool ConfigPanel::Init(void *window, int width, int height) {
  frame_width_ = width;
  frame_height_ = height;

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  ImGuiStyle *style = &ImGui::GetStyle();
  style->Alpha = 0.8f;

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL((GLFWwindow *) window, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  // load config
  return LoadConfig();
}

void ConfigPanel::OnDraw() {
  // Start the Dear ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // Settings window
  ImGui::Begin("Settings",
               nullptr,
               ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
                   | ImGuiWindowFlags_AlwaysAutoResize);
  DrawSettings();
  ImGui::SetWindowPos(ImVec2(frame_width_ - ImGui::GetWindowWidth(), 0));
  ImGui::End();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ConfigPanel::DrawSettings() {
  // renderer
  const char *rendererItems[] = {
      "Software",
      "OpenGL",
  };
  ImGui::Separator();
  ImGui::Text("renderer");
  ImGui::Combo("##renderer", &config_.renderer_type, rendererItems, IM_ARRAYSIZE(rendererItems));

  // reset camera
  ImGui::Separator();
  ImGui::Text("camera:");
  ImGui::SameLine();
  if (ImGui::Button("reset")) {
    ResetCamera();
  }

  // fps
  ImGui::Separator();
  ImGui::Text("fps: %.1f (%.2f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
  ImGui::Text("triangles: %zu", config_.triangle_count_);

  // model
  ImGui::Separator();
  ImGui::Text("load model");
  for (const auto &kv : model_paths_) {
    if (ImGui::RadioButton(kv.first.c_str(), config_.model_name == kv.first)) {
      ReloadModel(kv.first);
    }
  }

  // skybox
  ImGui::Separator();
  ImGui::Checkbox("skybox", &config_.show_skybox);

  if (config_.show_skybox) {
    for (const auto &kv : skybox_paths_) {
      if (ImGui::RadioButton(kv.first.c_str(), config_.skybox_name == kv.first)) {
        ReloadSkybox(kv.first);
      }
    }
  }

  // clear Color
  ImGui::Separator();
  ImGui::Text("clear color");
  ImGui::ColorEdit4("clear color", (float *) &config_.clear_color, ImGuiColorEditFlags_NoLabel);

  // wireframe
  ImGui::Separator();
  ImGui::Checkbox("wireframe", &config_.wireframe);
  ImGui::SameLine();
  ImGui::Checkbox("show clip", &config_.wireframe_show_clip);

  // world axis
  ImGui::Separator();
  ImGui::Checkbox("world axis", &config_.world_axis);

  if (config_.wireframe) {
    return;
  }

  // light
  ImGui::Separator();
  ImGui::Text("ambient color");
  ImGui::ColorEdit3("ambient color", (float *) &config_.ambient_color, ImGuiColorEditFlags_NoLabel);

  ImGui::Separator();
  ImGui::Checkbox("point light", &config_.show_light);
  if (config_.show_light) {
    ImGui::Text("color");
    ImGui::ColorEdit3("light color", (float *) &config_.point_light_color, ImGuiColorEditFlags_NoLabel);

    ImGui::Text("position");
    ImGui::SliderAngle("##light position", &light_position_angle_, 0, 360.f);
  }

  // face cull
  ImGui::Separator();
  ImGui::Checkbox("cull face", &config_.cull_face);

  // depth test
  ImGui::Separator();
  ImGui::Checkbox("depth test", &config_.depth_test);

  // early z
  ImGui::Separator();
  ImGui::Checkbox("early z", &config_.early_z);

  // Anti aliasing
  const char *aaItems[] = {
      "NONE",
      "SSAA",
      "FXAA",
  };
  ImGui::Separator();
  ImGui::Text("Anti-aliasing");
  ImGui::Combo("##Anti-aliasing", &config_.aa_type, aaItems, IM_ARRAYSIZE(aaItems));
}

void ConfigPanel::Destroy() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ConfigPanel::Update() {
  // update light position
  config_.point_light_position = 2.f * glm::vec3(glm::sin(light_position_angle_),
                                                 1.0f,
                                                 glm::cos(light_position_angle_));
  if (update_light_func_) {
    update_light_func_(config_.point_light_position, config_.point_light_color);
  }
}

void ConfigPanel::UpdateSize(int width, int height) {
  frame_width_ = width;
  frame_height_ = height;
}

bool ConfigPanel::WantCaptureKeyboard() {
  ImGuiIO &io = ImGui::GetIO();
  return io.WantCaptureKeyboard;
}

bool ConfigPanel::WantCaptureMouse() {
  ImGuiIO &io = ImGui::GetIO();
  return io.WantCaptureMouse;
}

bool ConfigPanel::LoadConfig() {
  auto config_path = ASSETS_DIR + "assets.json";
  std::string config_str = FileUtils::ReadAll(config_path);
  LOGD("load assets config: %s", config_path.c_str());

  std::string err;
  const auto json = json11::Json::parse(config_str, err);
  for (auto &kv : json["model"].object_items()) {
    model_paths_[kv.first] = ASSETS_DIR + kv.second["path"].string_value();
  }
  for (auto &kv : json["skybox"].object_items()) {
    skybox_paths_[kv.first] = ASSETS_DIR + kv.second["path"].string_value();
  }

  if (model_paths_.empty()) {
    LOGE("load models failed");
    return false;
  }

  // load default model & skybox
  return ReloadModel(model_paths_.begin()->first)
      && ReloadSkybox(skybox_paths_.begin()->first);
}

bool ConfigPanel::ReloadModel(const std::string &name) {
  if (name != config_.model_name) {
    config_.model_name = name;
    config_.model_path = model_paths_[config_.model_name];

    if (reload_model_func_) {
      return reload_model_func_(config_.model_path);
    }
  }

  return true;
}

bool ConfigPanel::ReloadSkybox(const std::string &name) {
  if (name != config_.skybox_name) {
    config_.skybox_name = name;
    config_.skybox_path = skybox_paths_[config_.skybox_name];

    if (reload_skybox_func_) {
      return reload_skybox_func_(config_.skybox_path);
    }
  }

  return true;
}

void ConfigPanel::ResetCamera() {
  if (reset_camera_func_) {
    reset_camera_func_();
  }
}

}
}
