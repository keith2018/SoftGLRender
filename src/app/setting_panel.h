/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "imgui/imgui.h"
#include "settings.h"

namespace SoftGL {
namespace App {

class SettingPanel {
 public:
  explicit SettingPanel(Settings &settings) : settings_(settings) {}

  void Init(void *window, int width, int height);
  void OnDraw();
  void Destroy();
  void UpdateSize(int width, int height);
  void ToggleShowState();

  bool WantCaptureKeyboard();
  bool WantCaptureMouse();

 private:
  void DrawSettings();

 private:
  Settings &settings_;

  bool visible = true;
  int frame_width_ = 0;
  int frame_height_ = 0;
};

}
}
