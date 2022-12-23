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

class ViewerSharedData {
 public:
  std::shared_ptr<Settings> settings_;
  std::shared_ptr<SettingPanel> settingPanel_;
  std::shared_ptr<Camera> camera_;
  std::shared_ptr<SmoothOrbitController> orbitController_;
  std::shared_ptr<ModelLoader> model_loader_;
};

class Viewer {
 public:
  virtual ~Viewer() { Destroy(); }
  virtual bool Create(void *window, int width, int height, int outTexId);
  virtual void DrawFrame();
  virtual void Destroy();

  inline void DrawUI() {
    viewer_->settingPanel_->OnDraw();
  }

  inline void UpdateSize(int width, int height) {
    viewer_->settingPanel_->UpdateSize(width, height);
  }

  inline void ToggleUIShowState() {
    viewer_->settingPanel_->ToggleShowState();
  }

  inline bool WantCaptureKeyboard() {
    return viewer_->settingPanel_->WantCaptureKeyboard();
  }

  inline bool WantCaptureMouse() {
    return viewer_->settingPanel_->WantCaptureMouse();
  }

  inline SmoothOrbitController *GetOrbitController() {
    return viewer_->orbitController_.get();
  }

  inline Settings *GetSettings() {
    return viewer_->settings_.get();
  }

 protected:
  int width_ = 0;
  int height_ = 0;
  int outTexId_ = 0;

  AAType aa_type_ = AAType_NONE;

  static std::shared_ptr<ViewerSharedData> viewer_;
};

}
}
