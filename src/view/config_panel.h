/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <functional>
#include <unordered_map>
#include "config.h"

namespace SoftGL {
namespace View {

const std::string ASSETS_DIR = "./assets/";

class ConfigPanel {
 public:
  explicit ConfigPanel(Config &config) : config_(config) {}
  ~ConfigPanel() { Destroy(); };

  bool Init(void *window, int width, int height);
  void OnDraw();

  void Update();
  void UpdateSize(int width, int height);

  bool WantCaptureKeyboard();
  bool WantCaptureMouse();

  inline void SetReloadModelFunc(const std::function<bool(const std::string &)> &func) {
    reload_model_func_ = func;
  }
  inline void SetReloadSkyboxFunc(const std::function<bool(const std::string &)> &func) {
    reload_skybox_func_ = func;
  }
  inline void SetUpdateLightFunc(const std::function<void(glm::vec3 &, glm::vec3 &)> &func) {
    update_light_func_ = func;
  }
  inline void SetResetCameraFunc(const std::function<void(void)> &func) {
    reset_camera_func_ = func;
  }

 private:
  bool LoadConfig();

  bool ReloadModel(const std::string &name);
  bool ReloadSkybox(const std::string &name);
  void ResetCamera();

  void DrawSettings();
  void Destroy();

 private:
  Config &config_;

  int frame_width_ = 0;
  int frame_height_ = 0;

  float light_position_angle_ = glm::radians(0.f);

  std::unordered_map<std::string, std::string> model_paths_;
  std::unordered_map<std::string, std::string> skybox_paths_;

  std::function<bool(const std::string &path)> reload_model_func_;
  std::function<bool(const std::string &path)> reload_skybox_func_;
  std::function<void(glm::vec3 &position, glm::vec3 &color)> update_light_func_;
  std::function<void(void)> reset_camera_func_;
};

}
}
