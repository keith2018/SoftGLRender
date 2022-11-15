/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "camera.h"
#include "orbit_controller.h"
#include "settings.h"
#include "setting_panel.h"

namespace SoftGL {
namespace View {

class Viewer {
 public:
  virtual bool Create(void *window, int width, int height, int outTexId);
  virtual void DrawFrame();

  inline void DrawUI() {
    settingPanel_->OnDraw();
  }

  inline void UpdateSize(int width, int height) {
    settingPanel_->UpdateSize(width, height);
  }

  inline void ToggleUIShowState() {
    settingPanel_->ToggleShowState();
  }

  inline bool WantCaptureKeyboard() {
    return settingPanel_->WantCaptureKeyboard();
  }

  inline bool WantCaptureMouse() {
    return settingPanel_->WantCaptureMouse();
  }

  inline SmoothOrbitController *GetOrbitController() {
    return orbitController_.get();
  }

 protected:
  int width_ = 0;
  int height_ = 0;
  int outTexId_ = 0;

  std::shared_ptr<Settings> settings_;
  std::shared_ptr<SettingPanel> settingPanel_;
  std::shared_ptr<Camera> camera_;
  std::shared_ptr<SmoothOrbitController> orbitController_;
  std::shared_ptr<ModelLoader> model_loader_;
};

}
}