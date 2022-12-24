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

  return true;
}

void ViewerOpenGL::DrawFrame() {
  Viewer::DrawFrame();
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0, 0, width_, height_);
  DrawFrameInternal();
}

void ViewerOpenGL::Destroy() {
  glDeleteFramebuffers(1, &fbo_);
  Viewer::Destroy();
}

void ViewerOpenGL::DrawFrameInternal() {
  if (viewer_->settings_->depth_test) {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
  } else {
    glDisable(GL_DEPTH_TEST);
  }

  if (viewer_->settings_->cull_face) {
    glEnable(GL_CULL_FACE);
  } else {
    glDisable(GL_CULL_FACE);
  }

  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);

  auto clear_color = viewer_->settings_->clear_color;
  renderer_->Create(width_, height_, viewer_->camera_->Near(), viewer_->camera_->Far());
  renderer_->Clear(clear_color.r, clear_color.g, clear_color.b, clear_color.a);

  auto &uniforms = renderer_->GetRendererUniforms();
  UpdateUniformLight(uniforms.uniforms_light);

  // world axis
  if (viewer_->settings_->world_axis && !viewer_->settings_->show_skybox) {
    glm::mat4 axis_transform(1.0f);
    ModelLines &axis_lines = viewer_->model_loader_->GetWorldAxisLines();
    DrawWorldAxis(axis_lines, axis_transform);
  }

  // light
  if (!viewer_->settings_->wireframe && viewer_->settings_->show_light) {
    glm::mat4 light_transform(1.0f);
    ModelPoints &light_points = viewer_->model_loader_->GetLights();
    DrawLights(light_points, light_transform);
  }

  // adjust model center
  glm::mat4 model_transform(1.0f);
  auto bounds = viewer_->model_loader_->GetRootBoundingBox();
  glm::vec3 trans = (bounds->max + bounds->min) / -2.f;
  trans.y = -bounds->min.y;
  float bounds_len = glm::length(bounds->max - bounds->min);
  model_transform = glm::scale(model_transform, glm::vec3(3.f / bounds_len));
  model_transform = glm::translate(model_transform, trans);

  // draw model nodes opaque
  ModelNode *model_node = viewer_->model_loader_->GetRootNode();
  if (model_node != nullptr) {
    // draw nodes opaque
    DrawModelNodes(*model_node, model_transform, Alpha_Opaque, viewer_->settings_->wireframe);
    DrawModelNodes(*model_node, model_transform, Alpha_Mask, viewer_->settings_->wireframe);
  }

  // skybox
  if (viewer_->settings_->show_skybox) {
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    glm::mat4 skybox_matrix(1.f);
    auto skybox_node = viewer_->model_loader_->GetSkyBoxMesh();
    if (skybox_node) {
      DrawSkybox(*skybox_node, skybox_matrix);
    }

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
  }

  // draw model nodes blend
  if (model_node != nullptr) {
    // draw nodes blend
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    DrawModelNodes(*model_node, model_transform, Alpha_Blend, viewer_->settings_->wireframe);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
  }

  // FXAA
}

bool ViewerOpenGL::CheckMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform) {
  BoundingBox bbox = mesh.bounding_box.Transform(transform);
  return viewer_->camera_->GetFrustum().Intersects(bbox);
}

void ViewerOpenGL::DrawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe) {
  glm::mat4 model_matrix = transform * node.transform;
  auto &uniforms = renderer_->GetRendererUniforms();
  UpdateUniformMVP(uniforms.uniforms_mvp, model_matrix);

  // draw nodes
  for (auto &mesh : node.meshes) {
    if (mesh.alpha_mode != mode) {
      continue;
    }

    // frustum cull
    bool mesh_inside = CheckMeshFrustumCull(mesh, model_matrix);
    if (!mesh_inside) {
      continue;
    }

    if (wireframe) {
      uniforms.uniforms_color.data.u_baseColor = glm::vec4(1.f);
      renderer_->DrawMeshWireframe(mesh);
    } else {
      renderer_->DrawMeshTextured(mesh);
    }
  }

  // draw child
  for (auto &child_node : node.children) {
    DrawModelNodes(child_node, model_matrix, mode, wireframe);
  }
}

void ViewerOpenGL::DrawSkybox(ModelMesh &mesh, glm::mat4 &transform) {
  auto &uniforms = renderer_->GetRendererUniforms();
  // only rotation
  glm::mat4 view_matrix = glm::mat3(viewer_->camera_->ViewMatrix());
  uniforms.uniforms_mvp.data.u_modelViewProjectionMatrix =
      viewer_->camera_->ProjectionMatrix() * view_matrix * transform;
  renderer_->DrawMeshTextured(mesh);
}

void ViewerOpenGL::DrawWorldAxis(ModelLines &lines, glm::mat4 &transform) {
  auto &uniforms = renderer_->GetRendererUniforms();
  UpdateUniformMVP(uniforms.uniforms_mvp, transform);
  uniforms.uniforms_color.data.u_baseColor = lines.line_color;
  renderer_->DrawLines(lines);
}

void ViewerOpenGL::DrawLights(ModelPoints &points, glm::mat4 &transform) {
  auto &uniforms = renderer_->GetRendererUniforms();
  UpdateUniformMVP(uniforms.uniforms_mvp, transform);
  uniforms.uniforms_color.data.u_baseColor = points.point_color;
  renderer_->DrawPoints(points);
}

void ViewerOpenGL::UpdateUniformMVP(UniformsMVP &uniforms, glm::mat4 &transform) {
  uniforms.data.u_modelMatrix = transform;
  uniforms.data.u_modelViewProjectionMatrix =
      viewer_->camera_->ProjectionMatrix() * viewer_->camera_->ViewMatrix() * transform;
  uniforms.data.u_inverseTransposeModelMatrix = glm::mat3(glm::transpose(glm::inverse(transform)));
}

void ViewerOpenGL::UpdateUniformLight(UniformsLight &uniforms) {
  uniforms.data.u_showPointLight = viewer_->settings_->show_light;
  uniforms.data.u_ambientColor = viewer_->settings_->ambient_color;
  uniforms.data.u_cameraPosition = viewer_->camera_->Eye();
  uniforms.data.u_pointLightPosition = viewer_->settings_->light_position;
  uniforms.data.u_pointLightColor = viewer_->settings_->light_color;
}

}
}