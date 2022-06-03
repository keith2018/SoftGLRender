/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "setting_panel.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

namespace SoftGL {
namespace App {

void SettingPanel::Init(void *window, int width, int height) {
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
  ImGui_ImplOpenGL3_Init("#version 150");
}

void SettingPanel::OnDraw() {
  if (!visible) {
    return;
  }

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

void SettingPanel::DrawSettings() {
  // reset camera
  ImGui::Separator();
  ImGui::Text("camera:");
  ImGui::SameLine();
  if (ImGui::Button("reset")) {
    settings_.ResetCamera();
  }

  // fps
  ImGui::Separator();
  ImGui::Text("fps: %.1f (%.2f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
  ImGui::Text("triangles: %zu", settings_.triangle_count_);

  // model
  ImGui::Separator();
  ImGui::Text("load model");
  for (const auto &kv : settings_.GetModelPaths()) {
    if (ImGui::RadioButton(kv.first.c_str(), settings_.model_name == kv.first)) {
      settings_.ReloadModel(kv.first);
    }
  }

  // skybox
  ImGui::Separator();
  ImGui::Checkbox("skybox", &settings_.show_skybox);

  if (settings_.show_skybox) {
    for (const auto &kv : settings_.GetSkyboxPaths()) {
      if (ImGui::RadioButton(kv.first.c_str(), settings_.skybox_name == kv.first)) {
        settings_.ReloadSkybox(kv.first);
      }
    }
  }

  // clear Color
  ImGui::Separator();
  ImGui::Text("clear color");
  ImGui::ColorEdit4("clear color", (float *) &settings_.clear_color, ImGuiColorEditFlags_NoLabel);

  // wireframe
  ImGui::Separator();
  ImGui::Checkbox("wireframe", &settings_.wireframe);
  ImGui::SameLine();
  ImGui::Checkbox("show clip", &settings_.wireframe_show_clip);

  // world axis
  ImGui::Separator();
  ImGui::Checkbox("world axis", &settings_.world_axis);

  if (settings_.wireframe) {
    return;
  }

  // light
  ImGui::Separator();
  ImGui::Text("ambient color");
  ImGui::ColorEdit3("ambient color", (float *) &settings_.ambient_color, ImGuiColorEditFlags_NoLabel);

  ImGui::Separator();
  ImGui::Checkbox("point light", &settings_.show_light);
  if (settings_.show_light) {
    ImGui::Text("color");
    ImGui::ColorEdit3("light color", (float *) &settings_.light_color, ImGuiColorEditFlags_NoLabel);

    ImGui::Text("position");
    ImGui::SliderAngle("##light position", &settings_.light_position_angle, 0, 360.f);
  }

  // face cull
  ImGui::Separator();
  ImGui::Checkbox("cull face", &settings_.cull_face);

  // depth test
  ImGui::Separator();
  ImGui::Checkbox("depth test", &settings_.depth_test);

  // early z
  ImGui::Separator();
  ImGui::Checkbox("early z", &settings_.early_z);

  // Anti aliasing
  const char *aaItems[] = {
      "NONE",
      "SSAA",
      "FXAA",
  };
  ImGui::Separator();
  ImGui::Text("Anti-aliasing");
  ImGui::Combo("##Anti-aliasing", &settings_.aa_type, aaItems, IM_ARRAYSIZE(aaItems));
}

void SettingPanel::Destroy() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void SettingPanel::UpdateSize(int width, int height) {
  frame_width_ = width;
  frame_height_ = height;
}

void SettingPanel::ToggleShowState() {
  visible = !visible;
}

bool SettingPanel::WantCaptureKeyboard() {
  ImGuiIO &io = ImGui::GetIO();
  return io.WantCaptureKeyboard;
}

bool SettingPanel::WantCaptureMouse() {
  ImGuiIO &io = ImGui::GetIO();
  return io.WantCaptureMouse;
}

}
}
