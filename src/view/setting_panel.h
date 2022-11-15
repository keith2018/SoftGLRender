/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "imgui/imgui.h"
#include "settings.h"

namespace SoftGL {
namespace View {

class SettingPanel {
 public:
  explicit SettingPanel(Settings &settings) : settings_(settings) {}
  ~SettingPanel() { Destroy(); };

  void Init(void *window, int width, int height);
  void OnDraw();

  void UpdateSize(int width, int height);
  void ToggleShowState();

  bool WantCaptureKeyboard();
  bool WantCaptureMouse();

 private:
  void DrawSettings();
  void Destroy();

 private:
  Settings &settings_;

  bool visible = true;
  int frame_width_ = 0;
  int frame_height_ = 0;
};

}
}
