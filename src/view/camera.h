/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/geometry.h"

namespace SoftGL {
namespace View {

class Camera {
 public:
  void SetPerspective(float fov, float aspect, float near, float far);

  void LookAt(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up);

  glm::mat4 ProjectionMatrix() const;
  glm::mat4 ViewMatrix() const;

  glm::vec3 GetWorldPositionFromView(glm::vec3 pos) const;

  void Update();
  inline const Frustum &GetFrustum() const { return frustum_; }

  inline const float &Fov() const { return fov_; }
  inline const float &Aspect() const { return aspect_; }
  inline const float &Near() const { return near_; }
  inline const float &Far() const { return far_; }

  inline const glm::vec3 &Eye() const { return eye_; }
  inline const glm::vec3 &Center() const { return center_; }
  inline const glm::vec3 &Up() const { return up_; }

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
