/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "orbit_controller.h"

#define MIN_ORBIT_ARM_LENGTH 1.f

namespace SoftGL {
namespace App {

static const glm::vec3 init_eye_(0, 1, 3);
static const glm::vec3 init_center_(0, 1, 0);
static const glm::vec3 init_up_(0, 1, 0);

OrbitController::OrbitController(Camera &camera) : camera_(camera) {
  Reset();
}

void OrbitController::Update() {
  eye_ = center_ + arm_dir_ * arm_length_;
  camera_.LookAt(eye_, center_, up_);
}

void OrbitController::PanByPixels(double offset_x, double offset_y) {
  glm::vec3 world_offset = camera_.GetWorldPositionFromView(glm::vec3(offset_x, -offset_y, 0));
  glm::vec3 world_origin = camera_.GetWorldPositionFromView(glm::vec3(0));

  glm::vec3 delta = (world_origin - world_offset) * arm_length_ * pan_sensitivity_;
  center_ += delta;
}

void OrbitController::RotateByPixels(double offset_x, double offset_y) {
  float x_angle = (float) offset_x * rotate_sensitivity_;
  float y_angle = (float) offset_y * rotate_sensitivity_;

  glm::qua<float> q = glm::qua<float>(glm::radians(glm::vec3(-y_angle, -x_angle, 0)));
  glm::vec3 new_dir = glm::mat4_cast(q) * glm::vec4(arm_dir_, 1.0f);

  arm_dir_ = glm::normalize(new_dir);
}

void OrbitController::ZoomByPixels(double dx, double dy) {
  arm_length_ += -(float) dy * zoom_sensitivity_;
  if (arm_length_ < MIN_ORBIT_ARM_LENGTH) {
    arm_length_ = MIN_ORBIT_ARM_LENGTH;
  }
  eye_ = center_ + arm_dir_ * arm_length_;
}

void OrbitController::Reset() {
  eye_ = init_eye_;
  center_ = init_center_;
  up_ = init_up_;

  glm::vec3 dir = eye_ - center_;
  arm_dir_ = glm::normalize(dir);
  arm_length_ = glm::length(dir);
}

}
}