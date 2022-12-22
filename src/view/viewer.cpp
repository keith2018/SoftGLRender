/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


#include "viewer.h"

namespace SoftGL {
namespace View {

bool Viewer::Create(void *window, int width, int height, int outTexId) {
  width_ = width;
  height_ = height;
  outTexId_ = outTexId;

  // settings
  settings_ = std::make_shared<Settings>();
  settingPanel_ = std::make_shared<SettingPanel>(*settings_);
  settingPanel_->Init(window, width, height);

  // camera
  camera_ = std::make_shared<Camera>();
  camera_->SetPerspective(glm::radians(60.f), (float) width / (float) height, 0.01f, 100.0f);

  // orbit controller
  auto orbitCtrl = std::make_shared<OrbitController>(*camera_);
  orbitController_ = std::make_shared<SmoothOrbitController>(orbitCtrl);
  settings_->BindOrbitController(*orbitCtrl);

  // model loader
  model_loader_ = std::make_shared<ModelLoader>();
  settings_->BindModelLoader(*model_loader_);

  // start load
  model_loader_->LoadSkyBoxTex(settings_->skybox_path);
  return model_loader_->LoadModel(settings_->model_path);
}

void Viewer::DrawFrame() {
  settings_->Update();
  camera_->Update();
  orbitController_->Update();
}

}
}