/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "camera.h"
#include "orbit_controller.h"
#include "config_panel.h"
#include "model_loader.h"
#include "viewer.h"

namespace SoftGL {
namespace View {

constexpr float CAMERA_FOV = 60.f;
constexpr float CAMERA_NEAR = 0.01f;
constexpr float CAMERA_FAR = 100.f;

class ViewerManager {
 public:
  bool Create(void *window, int width, int height, int outTexId) {
    window_ = window;
    width_ = width;
    height_ = height;
    outTexId_ = outTexId;

    // camera
    camera_ = std::make_shared<Camera>();
    camera_->SetPerspective(glm::radians(CAMERA_FOV), (float) width / (float) height, CAMERA_NEAR, CAMERA_FAR);

    // orbit controller
    orbit_controller_ = std::make_shared<SmoothOrbitController>(std::make_shared<OrbitController>(*camera_));

    // config
    config_ = std::make_shared<Config>();
    config_panel_ = std::make_shared<ConfigPanel>(*config_);
    config_panel_->SetResetCameraFunc([&]() -> void {
      orbit_controller_->Reset();
    });

    // viewer soft
    auto viewer_soft = std::make_shared<Viewer>(*config_, *camera_);
    viewer_soft->Create(width, height, outTexId);
    viewers_[Renderer_SOFT] = std::move(viewer_soft);

    // viewer opengl
    auto viewer_opengl = std::make_shared<Viewer>(*config_, *camera_);
    viewer_opengl->Create(width, height, outTexId);
    viewers_[Renderer_OPENGL] = std::move(viewer_opengl);

    // model loader
    model_loader_ = std::make_shared<ModelLoader>(*config_, *config_panel_);

    // init config
    return config_panel_->Init(window, width, height);
  }

  inline void DrawFrame() {
    camera_->Update();
    orbit_controller_->Update();
    config_panel_->Update();

    CurrentViewer().DrawFrame(model_loader_->GetScene());
  }

  inline void Destroy() {
    for (auto &it : viewers_) {
      it.second->Destroy();
    }
  }

  inline void DrawPanel() {
    if (show_config_panel_) {
      config_panel_->OnDraw();
    }
  }

  inline void TogglePanelState() {
    show_config_panel_ = !show_config_panel_;
  }

  inline void UpdateSize(int width, int height) {
    width_ = width;
    height_ = height;
    config_panel_->UpdateSize(width, height);
  }

  inline void UpdateGestureZoom(float x, float y) {
    orbit_controller_->zoomX = x;
    orbit_controller_->zoomY = y;
  }

  inline void UpdateGestureRotate(float x, float y) {
    orbit_controller_->rotateX = x;
    orbit_controller_->rotateY = y;
  }

  inline void UpdateGesturePan(float x, float y) {
    orbit_controller_->panX = x;
    orbit_controller_->panY = y;
  }

  inline bool WantCaptureKeyboard() {
    return config_panel_->WantCaptureKeyboard();
  }

  inline bool WantCaptureMouse() {
    return config_panel_->WantCaptureMouse();
  }

 private:
  inline Viewer &CurrentViewer() {
    return *viewers_[config_->renderer_type];
  }

 private:
  void *window_ = nullptr;
  int width_ = 0;
  int height_ = 0;
  int outTexId_ = 0;

  std::shared_ptr<Config> config_;
  std::shared_ptr<ConfigPanel> config_panel_;
  std::shared_ptr<Camera> camera_;
  std::shared_ptr<SmoothOrbitController> orbit_controller_;
  std::shared_ptr<ModelLoader> model_loader_;

  std::unordered_map<int, std::shared_ptr<Viewer>> viewers_;

  bool show_config_panel_ = true;
};

}
}
