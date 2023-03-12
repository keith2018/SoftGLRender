/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "Camera.h"
#include <algorithm>

namespace SoftGL {
namespace View {

void Camera::setPerspective(float fov, float aspect, float near, float far) {
  fov_ = fov;
  aspect_ = aspect;
  near_ = near;
  far_ = far;
}

void Camera::lookAt(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up) {
  eye_ = eye;
  center_ = center;
  up_ = up;
}

glm::mat4 Camera::projectionMatrix() const {
  float tanHalfFovInverse = 1.f / std::tan((fov_ * 0.5f));

  glm::mat4 projection(0.f);
  projection[0][0] = tanHalfFovInverse / aspect_;
  projection[1][1] = tanHalfFovInverse;
  projection[2][2] = -far_ / (far_ - near_);
  projection[3][2] = -(far_ * near_) / (far_ - near_);
  projection[2][3] = -1.f;

  return projection;
}

glm::mat4 Camera::viewMatrix() const {
  glm::vec3 forward(glm::normalize(center_ - eye_));
  glm::vec3 side(glm::normalize(cross(forward, up_)));
  glm::vec3 up(glm::cross(side, forward));

  glm::mat4 view(1.f);

  view[0][0] = side.x;
  view[1][0] = side.y;
  view[2][0] = side.z;
  view[3][0] = -glm::dot(side, eye_);

  view[0][1] = up.x;
  view[1][1] = up.y;
  view[2][1] = up.z;
  view[3][1] = -glm::dot(up, eye_);

  view[0][2] = -forward.x;
  view[1][2] = -forward.y;
  view[2][2] = -forward.z;
  view[3][2] = glm::dot(forward, eye_);

  return view;
}

glm::vec3 Camera::getWorldPositionFromView(glm::vec3 pos) const {
  glm::mat4 proj, view, projInv, viewInv;
  proj = projectionMatrix();
  view = viewMatrix();

  projInv = glm::inverse(proj);
  viewInv = glm::inverse(view);

  glm::vec4 pos_world = viewInv * projInv * glm::vec4(pos, 1);
  pos_world /= pos_world.w;

  return glm::vec3{pos_world};
}

void Camera::update() {
  glm::vec3 forward(glm::normalize(center_ - eye_));
  glm::vec3 side(glm::normalize(cross(forward, up_)));
  glm::vec3 up(glm::cross(side, forward));

  float nearHeightHalf = near_ * std::tan(fov_ / 2.f);
  float farHeightHalf = far_ * std::tan(fov_ / 2.f);
  float nearWidthHalf = nearHeightHalf * aspect_;
  float farWidthHalf = farHeightHalf * aspect_;

  // near plane
  glm::vec3 nearCenter = eye_ + forward * near_;
  glm::vec3 nearNormal = forward;
  frustum_.planes[0].set(nearNormal, nearCenter);

  // far plane
  glm::vec3 farCenter = eye_ + forward * far_;
  glm::vec3 farNormal = -forward;
  frustum_.planes[1].set(farNormal, farCenter);

  // top plane
  glm::vec3 topCenter = nearCenter + up * nearHeightHalf;
  glm::vec3 topNormal = glm::cross(glm::normalize(topCenter - eye_), side);
  frustum_.planes[2].set(topNormal, topCenter);

  // bottom plane
  glm::vec3 bottomCenter = nearCenter - up * nearHeightHalf;
  glm::vec3 bottomNormal = glm::cross(side, glm::normalize(bottomCenter - eye_));
  frustum_.planes[3].set(bottomNormal, bottomCenter);

  // left plane
  glm::vec3 leftCenter = nearCenter - side * nearWidthHalf;
  glm::vec3 leftNormal = glm::cross(glm::normalize(leftCenter - eye_), up);
  frustum_.planes[4].set(leftNormal, leftCenter);

  // right plane
  glm::vec3 rightCenter = nearCenter + side * nearWidthHalf;
  glm::vec3 rightNormal = glm::cross(up, glm::normalize(rightCenter - eye_));
  frustum_.planes[5].set(rightNormal, rightCenter);

  // 8 corners
  glm::vec3 nearTopLeft = nearCenter + up * nearHeightHalf - side * nearWidthHalf;
  glm::vec3 nearTopRight = nearCenter + up * nearHeightHalf + side * nearWidthHalf;
  glm::vec3 nearBottomLeft = nearCenter - up * nearHeightHalf - side * nearWidthHalf;
  glm::vec3 nearBottomRight = nearCenter - up * nearHeightHalf + side * nearWidthHalf;

  glm::vec3 farTopLeft = farCenter + up * farHeightHalf - side * farWidthHalf;
  glm::vec3 farTopRight = farCenter + up * farHeightHalf + side * farWidthHalf;
  glm::vec3 farBottomLeft = farCenter - up * farHeightHalf - side * farWidthHalf;
  glm::vec3 farBottomRight = farCenter - up * farHeightHalf + side * farWidthHalf;

  frustum_.corners[0] = nearTopLeft;
  frustum_.corners[1] = nearTopRight;
  frustum_.corners[2] = nearBottomLeft;
  frustum_.corners[3] = nearBottomRight;
  frustum_.corners[4] = farTopLeft;
  frustum_.corners[5] = farTopRight;
  frustum_.corners[6] = farBottomLeft;
  frustum_.corners[7] = farBottomRight;

  // bounding box
  frustum_.bbox.min = glm::vec3(std::numeric_limits<float>::max());
  frustum_.bbox.max = glm::vec3(std::numeric_limits<float>::min());
  for (auto &corner : frustum_.corners) {
    frustum_.bbox.min.x = std::min(frustum_.bbox.min.x, corner.x);
    frustum_.bbox.min.y = std::min(frustum_.bbox.min.y, corner.y);
    frustum_.bbox.min.z = std::min(frustum_.bbox.min.z, corner.z);

    frustum_.bbox.max.x = std::max(frustum_.bbox.max.x, corner.x);
    frustum_.bbox.max.y = std::max(frustum_.bbox.max.y, corner.y);
    frustum_.bbox.max.z = std::max(frustum_.bbox.max.z, corner.z);
  }
}

}
}
