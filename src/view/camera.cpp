/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "camera.h"

namespace SoftGL {
namespace View {

void Camera::SetPerspective(float fov, float aspect, float near, float far) {
  fov_ = fov;
  aspect_ = aspect;
  near_ = near;
  far_ = far;
}

void Camera::LookAt(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up) {
  eye_ = eye;
  center_ = center;
  up_ = up;
}

glm::mat4 Camera::ProjectionMatrix() const {
  float tanHalfFovInverse = 1.f / std::tan((fov_ * 0.5f));

  glm::mat4 projection(0.f);
  projection[0][0] = tanHalfFovInverse / aspect_;
  projection[1][1] = tanHalfFovInverse;
  projection[2][2] = -far_ / (far_ - near_);
  projection[3][2] = -(far_ * near_) / (far_ - near_);
  projection[2][3] = -1.f;

  return projection;
}

glm::mat4 Camera::ViewMatrix() const {
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

glm::vec3 Camera::GetWorldPositionFromView(glm::vec3 pos) const {
  glm::mat4 proj, view, proj_inv, view_inv;
  proj = ProjectionMatrix();
  view = ViewMatrix();

  proj_inv = glm::inverse(proj);
  view_inv = glm::inverse(view);

  glm::vec4 pos_world = view_inv * proj_inv * glm::vec4(pos, 1);
  pos_world /= pos_world.w;

  return glm::vec3{pos_world};
}

void Camera::Update() {
  glm::vec3 forward(glm::normalize(center_ - eye_));
  glm::vec3 side(glm::normalize(cross(forward, up_)));
  glm::vec3 up(glm::cross(side, forward));

  float near_height_half = near_ * std::tan(fov_ / 2.f);
  float far_height_half = far_ * std::tan(fov_ / 2.f);
  float near_width_half = near_height_half * aspect_;
  float far_width_half = far_height_half * aspect_;

  // near plane
  glm::vec3 near_center = eye_ + forward * near_;
  glm::vec3 near_normal = forward;
  frustum_.planes[0].Set(near_normal, near_center);

  // far plane
  glm::vec3 far_center = eye_ + forward * far_;
  glm::vec3 far_normal = -forward;
  frustum_.planes[1].Set(far_normal, far_center);

  // top plane
  glm::vec3 top_center = near_center + up * near_height_half;
  glm::vec3 top_normal = glm::cross(glm::normalize(top_center - eye_), side);
  frustum_.planes[2].Set(top_normal, top_center);

  // bottom plane
  glm::vec3 bottom_center = near_center - up * near_height_half;
  glm::vec3 bottom_normal = glm::cross(side, glm::normalize(bottom_center - eye_));
  frustum_.planes[3].Set(bottom_normal, bottom_center);

  // left plane
  glm::vec3 left_center = near_center - side * near_width_half;
  glm::vec3 left_normal = glm::cross(glm::normalize(left_center - eye_), up);
  frustum_.planes[4].Set(left_normal, left_center);

  // right plane
  glm::vec3 right_center = near_center + side * near_width_half;
  glm::vec3 right_normal = glm::cross(up, glm::normalize(right_center - eye_));
  frustum_.planes[5].Set(right_normal, right_center);

  // 8 corners
  glm::vec3 nearTopLeft = near_center + up * near_height_half - side * near_width_half;
  glm::vec3 nearTopRight = near_center + up * near_height_half + side * near_width_half;
  glm::vec3 nearBottomLeft = near_center - up * near_height_half - side * near_width_half;
  glm::vec3 nearBottomRight = near_center - up * near_height_half + side * near_width_half;

  glm::vec3 farTopLeft = far_center + up * far_height_half - side * far_width_half;
  glm::vec3 farTopRight = far_center + up * far_height_half + side * far_width_half;
  glm::vec3 farBottomLeft = far_center - up * far_height_half - side * far_width_half;
  glm::vec3 farBottomRight = far_center - up * far_height_half + side * far_width_half;

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
