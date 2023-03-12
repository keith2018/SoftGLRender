/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "bounding_box.h"
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

}