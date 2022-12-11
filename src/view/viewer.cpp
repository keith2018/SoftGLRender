/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


#include "viewer.h"

namespace SoftGL {
namespace View {

std::shared_ptr<ViewerSharedData> Viewer::viewer_ = nullptr;

bool Viewer::Create(void *window, int width, int height, int outTexId) {
  width_ = width;
  height_ = height;
  outTexId_ = outTexId;

  if (viewer_ == nullptr) {
    viewer_ = std::make_shared<ViewerSharedData>();

    // settings
    viewer_->settings_ = std::make_shared<Settings>();
    viewer_->settingPanel_ = std::make_shared<SettingPanel>(*viewer_->settings_);
    viewer_->settingPanel_->Init(window, width, height);

    // camera
    viewer_->camera_ = std::make_shared<Camera>();
    viewer_->camera_->SetPerspective(glm::radians(60.f), (float) width / (float) height, 0.01f, 100.0f);

    // orbit controller
    auto orbitCtrl = std::make_shared<OrbitController>(*viewer_->camera_);
    viewer_->orbitController_ = std::make_shared<SmoothOrbitController>(orbitCtrl);
    viewer_->settings_->BindOrbitController(*orbitCtrl);

    // model loader
    viewer_->model_loader_ = std::make_shared<ModelLoader>();
    viewer_->settings_->BindModelLoader(*viewer_->model_loader_);

    // start load
    viewer_->model_loader_->LoadSkyBoxTex(viewer_->settings_->skybox_path);
    return viewer_->model_loader_->LoadModel(viewer_->settings_->model_path);
  }

  return true;
}

void Viewer::DrawFrame() {
  viewer_->settings_->Update();
  viewer_->camera_->Update();
  viewer_->orbitController_->Update();
}

void Viewer::Destroy() {
  viewer_ = nullptr;
}

}
}
