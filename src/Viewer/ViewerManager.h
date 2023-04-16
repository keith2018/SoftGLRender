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

#define RENDER_TYPE_NONE (-1)

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
    modelLoader_ = std::make_shared<ModelLoader>(*config_);

    // setup config panel actions callback
    setupConfigPanelActions();

    // init config
    return configPanel_->init(window, width, height);
  }

  void setupConfigPanelActions() {
    configPanel_->setResetCameraFunc([&]() -> void {
      orbitController_->reset();
    });
    configPanel_->setResetMipmapsFunc([&]() -> void {
      waitRenderIdle();
      modelLoader_->getScene().model->resetStates();
    });
    configPanel_->setResetReverseZFunc([&]() -> void {
      waitRenderIdle();
      auto &viewer = viewers_[config_->rendererType];
      viewer->resetReverseZ();
    });
    configPanel_->setReloadModelFunc([&](const std::string &path) -> bool {
      waitRenderIdle();
      return modelLoader_->loadModel(path);
    });
    configPanel_->setReloadSkyboxFunc([&](const std::string &path) -> bool {
      waitRenderIdle();
      return modelLoader_->loadSkybox(path);
    });
    configPanel_->setUpdateLightFunc([&](glm::vec3 &position, glm::vec3 &color) -> void {
      auto &scene = modelLoader_->getScene();
      scene.pointLight.vertexes[0].a_position = position;
      scene.pointLight.UpdateVertexes();
      scene.pointLight.material->baseColor = glm::vec4(color, 1.f);
    });
  }

  int drawFrame() {
    orbitController_->update();
    camera_->update();
    configPanel_->update();

    // update triangle count
    config_->triangleCount_ = modelLoader_->getModelPrimitiveCnt();

    auto &viewer = viewers_[config_->rendererType];
    if (rendererType_ != config_->rendererType) {
      // change render type need to reset all model states
      resetStates();
      rendererType_ = config_->rendererType;
      viewer->create(width_, height_, outTexId_);
    }
    viewer->configRenderer();
    viewer->drawFrame(modelLoader_->getScene());
    return viewer->swapBuffer();
  }

  inline void destroy() {
    resetStates();
    for (auto &it : viewers_) {
      it.second->destroy();
    }
  }

  inline void waitRenderIdle() {
    if (rendererType_ != RENDER_TYPE_NONE) {
      viewers_[rendererType_]->waitRenderIdle();
    }
  }

  inline void resetStates() {
    waitRenderIdle();
    modelLoader_->resetAllModelStates();
    modelLoader_->getScene().resetStates();
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

  int rendererType_ = RENDER_TYPE_NONE;
  bool showConfigPanel_ = true;
};

}
}
