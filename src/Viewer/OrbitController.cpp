/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "OrbitController.h"

#define MIN_ORBIT_ARM_LENGTH 1.f

namespace SoftGL {
namespace View {

static const glm::vec3 init_eye_(-1.5, 3, 3);
static const glm::vec3 init_center_(0, 1, 0);
static const glm::vec3 init_up_(0, 1, 0);

OrbitController::OrbitController(Camera &camera) : camera_(camera) {
  reset();
}

void OrbitController::update() {
  eye_ = center_ + armDir_ * armLength_;
  camera_.lookAt(eye_, center_, up_);
}

void OrbitController::panByPixels(double dx, double dy) {
  glm::vec3 world_offset = camera_.getWorldPositionFromView(glm::vec3(dx, -dy, 0));
  glm::vec3 world_origin = camera_.getWorldPositionFromView(glm::vec3(0));

  glm::vec3 delta = (world_origin - world_offset) * armLength_ * panSensitivity_;
  center_ += delta;
}

void OrbitController::rotateByPixels(double dx, double dy) {
  float x_angle = (float) dx * rotateSensitivity_;
  float y_angle = (float) dy * rotateSensitivity_;

  glm::qua<float> q = glm::qua<float>(glm::radians(glm::vec3(-y_angle, -x_angle, 0)));
  glm::vec3 new_dir = glm::mat4_cast(q) * glm::vec4(armDir_, 1.0f);

  armDir_ = glm::normalize(new_dir);
}

void OrbitController::zoomByPixels(double dx, double dy) {
  armLength_ += -(float) dy * zoomSensitivity_;
  if (armLength_ < MIN_ORBIT_ARM_LENGTH) {
    armLength_ = MIN_ORBIT_ARM_LENGTH;
  }
  eye_ = center_ + armDir_ * armLength_;
}

void OrbitController::reset() {
  eye_ = init_eye_;
  center_ = init_center_;
  up_ = init_up_;

  glm::vec3 dir = eye_ - center_;
  armDir_ = glm::normalize(dir);
  armLength_ = glm::length(dir);
}

}
}