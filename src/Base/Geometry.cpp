/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "Geometry.h"
#include <algorithm>

namespace SoftGL {

void BoundingBox::getCorners(glm::vec3 *dst) const {
  dst[0] = glm::vec3(min.x, max.y, max.z);
  dst[1] = glm::vec3(min.x, min.y, max.z);
  dst[2] = glm::vec3(max.x, min.y, max.z);
  dst[3] = glm::vec3(max.x, max.y, max.z);

  dst[4] = glm::vec3(max.x, max.y, min.z);
  dst[5] = glm::vec3(max.x, min.y, min.z);
  dst[6] = glm::vec3(min.x, min.y, min.z);
  dst[7] = glm::vec3(min.x, max.y, min.z);
}

void BoundingBox::updateMinMax(glm::vec3 *point, glm::vec3 *min, glm::vec3 *max) {
  if (point->x < min->x) {
    min->x = point->x;
  }

  if (point->x > max->x) {
    max->x = point->x;
  }

  if (point->y < min->y) {
    min->y = point->y;
  }

  if (point->y > max->y) {
    max->y = point->y;
  }

  if (point->z < min->z) {
    min->z = point->z;
  }

  if (point->z > max->z) {
    max->z = point->z;
  }
}

BoundingBox BoundingBox::transform(const glm::mat4 &matrix) const {
  glm::vec3 corners[8];
  getCorners(corners);

  corners[0] = matrix * glm::vec4(corners[0], 1.f);
  glm::vec3 newMin = corners[0];
  glm::vec3 newMax = corners[0];
  for (int i = 1; i < 8; i++) {
    corners[i] = matrix * glm::vec4(corners[i], 1.f);
    updateMinMax(&corners[i], &newMin, &newMax);
  }
  return {newMin, newMax};
}

bool BoundingBox::intersects(const BoundingBox &box) const {
  return ((min.x >= box.min.x && min.x <= box.max.x) || (box.min.x >= min.x && box.min.x <= max.x)) &&
      ((min.y >= box.min.y && min.y <= box.max.y) || (box.min.y >= min.y && box.min.y <= max.y)) &&
      ((min.z >= box.min.z && min.z <= box.max.z) || (box.min.z >= min.z && box.min.z <= max.z));
}

void BoundingBox::merge(const BoundingBox &box) {
  min.x = std::min(min.x, box.min.x);
  min.y = std::min(min.y, box.min.y);
  min.z = std::min(min.z, box.min.z);

  max.x = std::max(max.x, box.max.x);
  max.y = std::max(max.y, box.max.y);
  max.z = std::max(max.z, box.max.z);
}

Plane::PlaneIntersects Plane::intersects(const BoundingBox &box) const {
  glm::vec3 center = (box.min + box.max) * 0.5f;
  glm::vec3 extent = (box.max - box.min) * 0.5f;
  float d = distance(center);
  float r = fabsf(extent.x * normal_.x) + fabsf(extent.y * normal_.y) + fabsf(extent.z * normal_.z);
  if (d == r) {
    return Plane::Intersects_Tangent;
  } else if (std::abs(d) < r) {
    return Plane::Intersects_Cross;
  }

  return (d > 0.0f) ? Plane::Intersects_Front : Plane::Intersects_Back;
}

Plane::PlaneIntersects Plane::intersects(const glm::vec3 &p0) const {
  float d = distance(p0);
  if (d == 0) {
    return Plane::Intersects_Tangent;
  }
  return (d > 0.0f) ? Plane::Intersects_Front : Plane::Intersects_Back;
}

Plane::PlaneIntersects Plane::intersects(const glm::vec3 &p0, const glm::vec3 &p1) const {
  Plane::PlaneIntersects state0 = intersects(p0);
  Plane::PlaneIntersects state1 = intersects(p1);

  if (state0 == state1) {
    return state0;
  }

  if (state0 == Plane::Intersects_Tangent || state1 == Plane::Intersects_Tangent) {
    return Plane::Intersects_Tangent;
  }

  return Plane::Intersects_Cross;
}

Plane::PlaneIntersects Plane::intersects(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2) const {
  Plane::PlaneIntersects state0 = intersects(p0, p1);
  Plane::PlaneIntersects state1 = intersects(p0, p2);
  Plane::PlaneIntersects state2 = intersects(p1, p2);

  if (state0 == state1 && state0 == state2) {
    return state0;
  }

  if (state0 == Plane::Intersects_Cross || state1 == Plane::Intersects_Cross || state2 == Plane::Intersects_Cross) {
    return Plane::Intersects_Cross;
  }

  return Plane::Intersects_Tangent;
}

bool Frustum::intersects(const BoundingBox &box) const {
  for (auto &plane : planes) {
    if (plane.intersects(box) == Plane::Intersects_Back) {
      return false;
    }
  }

  // check box intersects
  if (!bbox.intersects(box)) {
    return false;
  }

  return true;
}

bool Frustum::intersects(const glm::vec3 &p0) const {
  for (auto &plane : planes) {
    if (plane.intersects(p0) == Plane::Intersects_Back) {
      return false;
    }
  }

  return true;
}

bool Frustum::intersects(const glm::vec3 &p0, const glm::vec3 &p1) const {
  for (auto &plane : planes) {
    if (plane.intersects(p0, p1) == Plane::Intersects_Back) {
      return false;
    }
  }

  return true;
}

bool Frustum::intersects(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2) const {
  for (auto &plane : planes) {
    if (plane.intersects(p0, p1, p2) == Plane::Intersects_Back) {
      return false;
    }
  }

  return true;
}

}