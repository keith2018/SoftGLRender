/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "Graphic.h"

namespace SoftGL {

bool Graphic::Barycentric(glm::aligned_vec4 *vert,
                          glm::aligned_vec4 &v0,
                          glm::aligned_vec4 &p,
                          glm::aligned_vec4 &bc) {
#ifdef SOFTGL_SIMD_OPT
  // Ref: https://geometrian.com/programming/tutorials/cross-product/index.php
  __m128 vec0 = _mm_sub_ps(_mm_load_ps(&vert[0].x), _mm_set_ps(0, p.x + 0.5f, v0.x, v0.x));
  __m128 vec1 = _mm_sub_ps(_mm_load_ps(&vert[1].x), _mm_set_ps(0, p.y + 0.5f, v0.y, v0.y));

  __m128 tmp0 = _mm_shuffle_ps(vec0, vec0, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 tmp1 = _mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(3, 1, 0, 2));
  __m128 tmp2 = _mm_mul_ps(tmp0, vec1);
  __m128 tmp3 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 u = _mm_sub_ps(_mm_mul_ps(tmp0, tmp1), tmp3);

  if (std::abs(MM_F32(u, 2)) < FLT_EPSILON) {
    return false;
  }

  u = _mm_div_ps(u, _mm_set1_ps(MM_F32(u, 2)));
  bc = {1.f - (MM_F32(u, 0) + MM_F32(u, 1)), MM_F32(u, 1), MM_F32(u, 0), 0.f};
#else
  glm::vec3 u = glm::cross(glm::vec3(vert[0]) - glm::vec3(v0.x, v0.x, p.x + 0.5f),
                           glm::vec3(vert[1]) - glm::vec3(v0.y, v0.y, p.y + 0.5f));
  if (std::abs(u.z) < FLT_EPSILON) {
    return false;
  }

  u /= u.z;
  bc = {1.f - (u.x + u.y), u.y, u.x, 0.f};
#endif

  if (bc.x < 0 || bc.y < 0 || bc.z < 0) {
    return false;
  }

  return true;
}

void Graphic::DrawLine(glm::vec4 v0, glm::vec4 v1, const glm::u8vec4 &color) {
  int x0 = (int) v0.x, y0 = (int) v0.y;
  int x1 = (int) v1.x, y1 = (int) v1.y;

  float z0 = v0.z;
  float z1 = v1.z;

  bool steep = false;
  if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
    std::swap(x0, y0);
    std::swap(x1, y1);
    steep = true;
  }
  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
    std::swap(z0, z1);
  }
  int dx = x1 - x0;
  int dy = y1 - y0;
  float dz = (x1 == x0) ? 0 : (z1 - z0) / (float) (x1 - x0);

  int error = 0;
  int dError = 2 * std::abs(dy);

  int y = y0;
  float z = z0;

  for (int x = x0; x <= x1; x++) {
    z += dz;
    if (steep) {
      DrawPoint(y, x, z, color);
    } else {
      DrawPoint(x, y, z, color);
    }

    error += dError;
    if (error > dx) {
      y += (y1 > y0 ? 1 : -1);
      error -= 2 * dx;
    }
  }
}

void Graphic::DrawCircle(int xc, int yc, int r, float depth, const glm::u8vec4 &color, bool fill) {
  int x = 0, y = r, yi, d;
  d = 3 - 2 * r;

  if (fill) {
    while (x <= y) {
      for (yi = x; yi <= y; yi++)
        DrawCirclePoint(xc, yc, x, yi, depth, color);

      if (d < 0) {
        d = d + 4 * x + 6;
      } else {
        d = d + 4 * (x - y) + 10;
        y--;
      }
      x++;
    }
  } else {
    while (x <= y) {
      DrawCirclePoint(xc, yc, x, y, depth, color);

      if (d < 0) {
        d = d + 4 * x + 6;
      } else {
        d = d + 4 * (x - y) + 10;
        y--;
      }
      x++;
    }
  }
}

void Graphic::DrawCirclePoint(int xc, int yc, int x, int y, float depth, const glm::u8vec4 &color) {
  DrawPoint(xc + x, yc + y, depth, color);
  DrawPoint(xc - x, yc + y, depth, color);
  DrawPoint(xc + x, yc - y, depth, color);
  DrawPoint(xc - x, yc - y, depth, color);
  DrawPoint(xc + y, yc + x, depth, color);
  DrawPoint(xc - y, yc + x, depth, color);
  DrawPoint(xc + y, yc - x, depth, color);
  DrawPoint(xc - y, yc - x, depth, color);
}

}
