/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Camera.h"
#include "OrbitController.h"
#include "ConfigPanel.h"
#include "ModelLoader.h"
#include "ViewerSoftware.h"
#include "ViewerOpenGL.h"
#include "ViewerVulkan.h"

namespace SoftGL {
namespace View {

class ViewerManager {
 public:
  bool create(void *window, int width, int height, int outTexId) {
    window_ = window;
    width_ = width;
    height_ = height;
    outTexId_ = outTexId;

    // camera
    camera_ = std::make_shared<Camera>();
    camera_->setPerspective(glm::radians(CAMERA_FOV), (float) width / (float) height, CAMERA_NEAR, CAMERA_FAR);

    // orbit controller
    orbitController_ = std::make_shared<SmoothOrbitController>(std::make_shared<OrbitController>(*camera_));

    // config
    config_ = std::make_shared<Config>();
    configPanel_ = std::make_shared<ConfigPanel>(*config_);
    configPanel_->setResetCameraFunc([&]() -> void {
      orbitController_->reset();
    });
    configPanel_->setResetMipmapsFunc([&]() -> void {
      modelLoader_->getScene().model->resetStates();
    });
    configPanel_->setResetReverseZFunc([&]() -> void {
      auto &viewer = viewers_[config_->rendererType];
      viewer->onResetReverseZ();
    });

    // viewer software
    auto viewer_soft = std::make_shared<ViewerSoftware>(*config_, *camera_);
    viewers_[Renderer_SOFT] = std::move(viewer_soft);

    // viewer opengl
    auto viewer_opengl = std::make_shared<ViewerOpenGL>(*config_, *camera_);
    viewers_[Renderer_OPENGL] = std::move(viewer_opengl);

    // viewer vulkan
    auto viewer_vulkan = std::make_shared<ViewerVulkan>(*config_, *camera_);
    viewers_[Renderer_Vulkan] = std::move(viewer_vulkan);

    // model loader
    modelLoader_ = std::make_shared<ModelLoader>(*config_, *configPanel_);

    // init config
    return configPanel_->init(window, width, height);
  }

  void drawFrame() {
    orbitController_->update();
    camera_->update();
    configPanel_->update();

    // update triangle count
    config_->triangleCount_ = modelLoader_->getModelPrimitiveCnt();

    auto &viewer = viewers_[config_->rendererType];
    if (rendererType_ != config_->rendererType) {
      rendererType_ = config_->rendererType;
      modelLoader_->resetAllModelStates();
      modelLoader_->getScene().resetStates();
      viewer->create(width_, height_, outTexId_);
    }
    viewer->configRenderer();
    viewer->drawFrame(modelLoader_->getScene());
    viewer->swapBuffer();
  }

  inline void destroy() {
    modelLoader_->resetAllModelStates();
    modelLoader_->getScene().resetStates();
    for (auto &it : viewers_) {
      it.second->destroy();
    }
  }

  inline void drawPanel() {
    if (showConfigPanel_) {
      configPanel_->onDraw();
    }
  }

  inline void togglePanelState() {
    showConfigPanel_ = !showConfigPanel_;
  }

  inline void updateSize(int width, int height) {
    width_ = width;
    height_ = height;
    configPanel_->updateSize(width, height);
  }

  inline void updateGestureZoom(float x, float y) {
    orbitController_->zoomX = x;
    orbitController_->zoomY = y;
  }

  inline void updateGestureRotate(float x, float y) {
    orbitController_->rotateX = x;
    orbitController_->rotateY = y;
  }

  inline void updateGesturePan(float x, float y) {
    orbitController_->panX = x;
    orbitController_->panY = y;
  }

  inline bool wantCaptureKeyboard() {
    return configPanel_->wantCaptureKeyboard();
  }

  inline bool wantCaptureMouse() {
    return configPanel_->wantCaptureMouse();
  }

 private:
  void *window_ = nullptr;
  int width_ = 0;
  int height_ = 0;
  int outTexId_ = 0;

  std::shared_ptr<Config> config_;
  std::shared_ptr<ConfigPanel> configPanel_;
  std::shared_ptr<Camera> camera_;
  std::shared_ptr<SmoothOrbitController> orbitController_;
  std::shared_ptr<ModelLoader> modelLoader_;

  std::unordered_map<int, std::shared_ptr<Viewer>> viewers_;

  int rendererType_ = -1;
  bool showConfigPanel_ = true;
};

}
}
