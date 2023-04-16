/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <vector>
#include <functional>
#include <unordered_map>
#include "Config.h"

namespace SoftGL {
namespace View {

class ConfigPanel {
 public:
  explicit ConfigPanel(Config &config) : config_(config) {}
  ~ConfigPanel() { destroy(); };

  bool init(void *window, int width, int height);
  void onDraw();

  void update();
  void updateSize(int width, int height);

  bool wantCaptureKeyboard();
  bool wantCaptureMouse();

  inline void setReloadModelFunc(const std::function<bool(const std::string &)> &func) {
    reloadModelFunc_ = func;
  }
  inline void setReloadSkyboxFunc(const std::function<bool(const std::string &)> &func) {
    reloadSkyboxFunc_ = func;
  }
  inline void setUpdateLightFunc(const std::function<void(glm::vec3 &, glm::vec3 &)> &func) {
    updateLightFunc_ = func;
  }
  inline void setResetCameraFunc(const std::function<void(void)> &func) {
    resetCameraFunc_ = func;
  }
  inline void setResetMipmapsFunc(const std::function<void(void)> &func) {
    resetMipmapsFunc_ = func;
  }
  inline void setResetReverseZFunc(const std::function<void(void)> &func) {
    resetReverseZFunc_ = func;
  }

 private:
  bool loadConfig();

  bool reloadModel(const std::string &name);
  bool reloadSkybox(const std::string &name);

  void drawSettings();
  void destroy();

 private:
  Config &config_;

  int frameWidth_ = 0;
  int frameHeight_ = 0;

  float lightPositionAngle_ = glm::radians(235.f);

  std::unordered_map<std::string, std::string> modelPaths_;
  std::unordered_map<std::string, std::string> skyboxPaths_;

  std::vector<const char *> modelNames_;
  std::vector<const char *> skyboxNames_;

  std::function<bool(const std::string &path)> reloadModelFunc_;
  std::function<bool(const std::string &path)> reloadSkyboxFunc_;
  std::function<void(glm::vec3 &position, glm::vec3 &color)> updateLightFunc_;
  std::function<void(void)> resetCameraFunc_;
  std::function<void(void)> resetMipmapsFunc_;
  std::function<void(void)> resetReverseZFunc_;
};

}
}
