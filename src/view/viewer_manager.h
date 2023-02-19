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
#include "viewer_opengl.h"
#include "viewer_soft.h"

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
    auto viewer_soft = std::make_shared<ViewerSoft>(*config_, *camera_);
    viewers_[Renderer_SOFT] = std::move(viewer_soft);

    // viewer opengl
    auto viewer_opengl = std::make_shared<ViewerOpenGL>(*config_, *camera_);
    viewers_[Renderer_OPENGL] = std::move(viewer_opengl);

    // model loader
    model_loader_ = std::make_shared<ModelLoader>(*config_, *config_panel_);

    // init config
    return config_panel_->Init(window, width, height);
  }

  void DrawFrame() {
    orbit_controller_->Update();
    camera_->Update();
    config_panel_->Update();

    // update triangle count
    config_->triangle_count_ = model_loader_->GetModelPrimitiveCnt();

    auto &viewer = viewers_[config_->renderer_type];
    if (renderer_type_ != config_->renderer_type) {
      renderer_type_ = config_->renderer_type;
      ResetSceneStates(model_loader_->GetScene());
      viewer->Create(width_, height_, outTexId_);
    }
    viewer->DrawFrame(model_loader_->GetScene());
    viewer->SwapBuffer();
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
  void ResetSceneStates(DemoScene &scene) {
    std::function<void(ModelNode &node)> reset_node_func = [&](ModelNode &node) -> void {
      for (auto &mesh : node.meshes) {
        mesh.vao = nullptr;
        mesh.material_wireframe.ResetRuntimeStates();
        mesh.material_textured.ResetRuntimeStates();
      }
      for (auto &child_node : node.children) {
        reset_node_func(child_node);
      }
    };
    reset_node_func(scene.model->root_node);
    scene.world_axis.vao = nullptr;
    scene.world_axis.material.ResetRuntimeStates();

    scene.point_light.vao = nullptr;
    scene.point_light.material.ResetRuntimeStates();

    scene.skybox.vao = nullptr;
    for (auto &kv : scene.skybox.material_cache) {
      kv.second.ResetRuntimeStates();
    }
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

  int renderer_type_ = -1;
  bool show_config_panel_ = true;
};

}
}
