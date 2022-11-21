/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


#include "viewer_opengl.h"

namespace SoftGL {
namespace View {

bool ViewerOpenGL::Create(void *window, int width, int height, int outTexId) {
  bool success = Viewer::Create(window, width, height, outTexId);
  if (!success) {
    return false;
  }

  renderer_ = std::make_shared<RendererOpenGL>();

  GLint lastFbo = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &lastFbo);

  // create fbo
  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

  // depth buffer
  GLuint depthRenderBuffer;
  glGenRenderbuffers(1, &depthRenderBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer);

  // color buffer
  glBindTexture(GL_TEXTURE_2D, outTexId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, outTexId, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    return false;
  }

  glViewport(0, 0, width_, height_);
  glBindFramebuffer(GL_FRAMEBUFFER, lastFbo);
  return true;
}

void ViewerOpenGL::DrawFrame() {
  Viewer::DrawFrame();

  GLint lastFbo = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &lastFbo);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  DrawFrameInternal();

  glBindFramebuffer(GL_FRAMEBUFFER, lastFbo);
}

void ViewerOpenGL::Destroy() {
  glDeleteFramebuffers(1, &fbo_);
}

void ViewerOpenGL::DrawFrameInternal() {
  auto clear_color = settings_->clear_color;
  renderer_->Clear(clear_color.r, clear_color.g, clear_color.b, clear_color.a);

  // world axis
  if (settings_->world_axis && !settings_->show_skybox) {
    glm::mat4 axis_transform(1.0f);
    ModelLines &axis_lines = model_loader_->GetWorldAxisLines();
    DrawWorldAxis(axis_lines, axis_transform);
  }

  // light
  if (!settings_->wireframe && settings_->show_light) {
    glm::mat4 light_transform(1.0f);
    ModelPoints &light_points = model_loader_->GetLights();
    DrawLights(light_points, light_transform);
  }

  // model root
  // model root
  ModelNode *model_node = model_loader_->GetRootNode();
  glm::mat4 model_transform(1.0f);
  if (model_node != nullptr) {
    // adjust model center
    auto bounds = model_loader_->GetRootBoundingBox();
    glm::vec3 trans = (bounds->max + bounds->min) / -2.f;
    trans.y = -bounds->min.y;
    float bounds_len = glm::length(bounds->max - bounds->min);
    model_transform = glm::scale(model_transform, glm::vec3(3.f / bounds_len));
    model_transform = glm::translate(model_transform, trans);

    // draw nodes opaque
    DrawModelNodes(*model_node, model_transform, Alpha_Opaque, settings_->wireframe);
    DrawModelNodes(*model_node, model_transform, Alpha_Mask, settings_->wireframe);
  }

  // skybox

  // FXAA
}

bool ViewerOpenGL::CheckMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform) {
  BoundingBox bbox = mesh.bounding_box.Transform(transform);
  return camera_->GetFrustum().Intersects(bbox);
}

void ViewerOpenGL::DrawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe) {

}

void ViewerOpenGL::DrawSkybox(ModelMesh &mesh, glm::mat4 &transform) {

}

void ViewerOpenGL::DrawWorldAxis(ModelLines &lines, glm::mat4 &transform) {
  auto &uniforms = renderer_->GetLineUniforms();
  uniforms.u_modelViewProjectionMatrix = camera_->ProjectionMatrix() * camera_->ViewMatrix() * transform;
  uniforms.u_fragColor = lines.line_color;
  renderer_->DrawLines(lines);
}

void ViewerOpenGL::DrawLights(ModelPoints &points, glm::mat4 &transform) {
  auto &uniforms = renderer_->GetPointUniforms();
  uniforms.u_modelViewProjectionMatrix = camera_->ProjectionMatrix() * camera_->ViewMatrix() * transform;
  uniforms.u_fragColor = points.point_color;
  renderer_->DrawPoints(points);
}


}
}