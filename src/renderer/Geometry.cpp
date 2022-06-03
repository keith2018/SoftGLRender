/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "geometry.h"

namespace SoftGL {

Plane::PlaneIntersects Plane::Intersects(const BoundingBox &box) const {
  glm::vec3 center = (box.min + box.max) * 0.5f;
  glm::vec3 extent = (box.max - box.min) * 0.5f;
  float distance = Distance(center);
  float r = fabsf(extent.x * normal_.x) + fabsf(extent.y * normal_.y) + fabsf(extent.z * normal_.z);
  if (distance == r) {
    return Plane::Intersects_Tangent;
  } else if (fabs(distance) < r) {
    return Plane::Intersects_Cross;
  }

  return (distance > 0.0f) ? Plane::Intersects_Front : Plane::Intersects_Back;
}

Plane::PlaneIntersects Plane::Intersects(const glm::vec3 &p0) const {
  float distance = Distance(p0);
  if (distance == 0) {
    return Plane::Intersects_Tangent;
  }
  return (distance > 0.0f) ? Plane::Intersects_Front : Plane::Intersects_Back;
}

Plane::PlaneIntersects Plane::Intersects(const glm::vec3 &p0, const glm::vec3 &p1) const {
  Plane::PlaneIntersects state0 = Intersects(p0);
  Plane::PlaneIntersects state1 = Intersects(p1);

  if (state0 == state1) {
    return state0;
  }

  if (state0 == Plane::Intersects_Tangent || state1 == Plane::Intersects_Tangent) {
    return Plane::Intersects_Tangent;
  }

  return Plane::Intersects_Cross;
}

Plane::PlaneIntersects Plane::Intersects(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2) const {
  Plane::PlaneIntersects state0 = Intersects(p0, p1);
  Plane::PlaneIntersects state1 = Intersects(p0, p2);
  Plane::PlaneIntersects state2 = Intersects(p1, p2);

  if (state0 == state1 && state0 == state2) {
    return state0;
  }

  if (state0 == Plane::Intersects_Cross || state1 == Plane::Intersects_Cross || state2 == Plane::Intersects_Cross) {
    return Plane::Intersects_Cross;
  }

  return Plane::Intersects_Tangent;
}

bool Frustum::Intersects(const BoundingBox &box) const {
  for (auto &plane : planes) {
    if (plane.Intersects(box) == Plane::Intersects_Back) return false;
  }

  // check box intersects
  if (!bbox.Intersects(box)) {
    return false;
  }

  return true;
}

bool Frustum::Intersects(const glm::vec3 &p0) const {
  for (auto &plane : planes) {
    if (plane.Intersects(p0) == Plane::Intersects_Back) {
      return false;
    }
  }

  return true;
}

bool Frustum::Intersects(const glm::vec3 &p0, const glm::vec3 &p1) const {
  for (auto &plane : planes) {
    if (plane.Intersects(p0, p1) == Plane::Intersects_Back) {
      return false;
    }
  }

  return true;
}

bool Frustum::Intersects(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2) const {
  for (auto &plane : planes) {
    if (plane.Intersects(p0, p1, p2) == Plane::Intersects_Back) {
      return false;
    }
  }

  return true;
}

}