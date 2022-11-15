/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "quad_filter.h"

namespace SoftGL {
namespace View {

QuadFilter::QuadFilter(int width, int height) : width_(width), height_(height) {
  quad_mesh_.face_cnt = 2;
  quad_mesh_.vertexes.push_back({{1.f, -1.f, 0.f}, {1.f, 0.f}});
  quad_mesh_.vertexes.push_back({{-1.f, -1.f, 0.f}, {0.f, 0.f}});
  quad_mesh_.vertexes.push_back({{1.f, 1.f, 0.f}, {1.f, 1.f}});
  quad_mesh_.vertexes.push_back({{-1.f, 1.f, 0.f}, {0.f, 1.f}});
  quad_mesh_.indices = {0, 1, 2, 1, 2, 3};

  renderer_ = std::make_shared<RendererSoft>();
  renderer_->Create(width_, height_, 0.f, 1.f);
  renderer_->depth_test = false;
  renderer_->frustum_clip = false;
  renderer_->cull_face_back = false;
}

void QuadFilter::Clear(float r, float g, float b, float a) {
  renderer_->Clear(r, g, b, a);
}

void QuadFilter::Draw() {
  glm::mat4 transform(1.0f);
  renderer_->DrawMeshTextured(quad_mesh_, transform);
}

}
}
