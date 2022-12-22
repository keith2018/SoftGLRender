/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "camera.h"

namespace SoftGL {
namespace View {

class OrbitController {
 public:
  explicit OrbitController(Camera &camera);

  void Update();

  void PanByPixels(double dx, double dy);

  void RotateByPixels(double dx, double dy);

  void ZoomByPixels(double dx, double dy);

  void Reset();

 private:
  Camera &camera_;

  glm::vec3 eye_{};
  glm::vec3 center_{};
  glm::vec3 up_{};

  float arm_length_;
  glm::vec3 arm_dir_{};

  float pan_sensitivity_ = 0.1f;
  float zoom_sensitivity_ = 0.2f;
  float rotate_sensitivity_ = 0.2f;
};

class SmoothOrbitController {
  const double motion_eps = 0.001f;
  const double motion_sensitivity = 1.2f;
 public:
  explicit SmoothOrbitController(std::shared_ptr<OrbitController> orbit_controller)
      : orbitController(std::move(orbit_controller)) {}

  void Update() {
    if (std::abs(zoomX) > motion_eps || std::abs(zoomY) > motion_eps) {
      zoomX /= motion_sensitivity;
      zoomY /= motion_sensitivity;
      orbitController->ZoomByPixels(zoomX, zoomY);
    } else {
      zoomX = 0;
      zoomY = 0;
    }

    if (std::abs(rotateX) > motion_eps || std::abs(rotateY) > motion_eps) {
      rotateX /= motion_sensitivity;
      rotateY /= motion_sensitivity;
      orbitController->RotateByPixels(rotateX, rotateY);
    } else {
      rotateX = 0;
      rotateY = 0;
    }

    if (std::abs(panX) > motion_eps || std::abs(panY) > motion_eps) {
      orbitController->PanByPixels(panX, panY);
      panX = 0;
      panY = 0;
    }

    orbitController->Update();
  }

  double zoomX = 0;
  double zoomY = 0;
  double rotateX = 0;
  double rotateY = 0;
  double panX = 0;
  double panY = 0;

 private:
  std::shared_ptr<OrbitController> orbitController;
};

}
}
