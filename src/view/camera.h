/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/geometry.h"

namespace SoftGL {
namespace View {

constexpr float CAMERA_FOV = 60.f;
constexpr float CAMERA_NEAR = 0.01f;
constexpr float CAMERA_FAR = 100.f;

class Camera {
 public:
  void update();

  void setPerspective(float fov, float aspect, float near, float far);

  void lookAt(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up);

  glm::mat4 projectionMatrix() const;
  glm::mat4 viewMatrix() const;

  glm::vec3 getWorldPositionFromView(glm::vec3 pos) const;

  inline const Frustum &getFrustum() const { return frustum_; }

  inline const float &fov() const { return fov_; }
  inline const float &aspect() const { return aspect_; }
  inline const float &near() const { return near_; }
  inline const float &far() const { return far_; }

  inline const glm::vec3 &eye() const { return eye_; }
  inline const glm::vec3 &center() const { return center_; }
  inline const glm::vec3 &up() const { return up_; }

 private:
  float fov_ = glm::radians(60.f);
  float aspect_ = 1.0f;
  float near_ = 0.01f;
  float far_ = 100.f;

  glm::vec3 eye_{};
  glm::vec3 center_{};
  glm::vec3 up_{};

  Frustum frustum_;
};

}
}
