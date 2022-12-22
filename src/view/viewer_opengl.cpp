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
}

void ViewerOpenGL::DrawFrameInternal() {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (settings_->depth_test) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }
  glDepthMask(GL_TRUE);

  if (settings_->cull_face) {
    glEnable(GL_CULL_FACE);
  } else {
    glDisable(GL_CULL_FACE);
  }

  auto clear_color = settings_->clear_color;
  renderer_->Create(width_, height_, camera_->Near(), camera_->Far());
  renderer_->Clear(clear_color.r, clear_color.g, clear_color.b, clear_color.a);

  auto &uniforms = renderer_->GetRendererUniforms();
  UpdateUniformLight(uniforms.uniforms_light);

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

  // adjust model center
  glm::mat4 model_transform(1.0f);
  auto bounds = model_loader_->GetRootBoundingBox();
  glm::vec3 trans = (bounds->max + bounds->min) / -2.f;
  trans.y = -bounds->min.y;
  float bounds_len = glm::length(bounds->max - bounds->min);
  model_transform = glm::scale(model_transform, glm::vec3(3.f / bounds_len));
  model_transform = glm::translate(model_transform, trans);

  // draw model nodes opaque
  ModelNode *model_node = model_loader_->GetRootNode();
  if (model_node != nullptr) {
    // draw nodes opaque
    DrawModelNodes(*model_node, model_transform, Alpha_Opaque, settings_->wireframe);
    DrawModelNodes(*model_node, model_transform, Alpha_Mask, settings_->wireframe);
  }

  // skybox

  // draw model nodes blend
  if (model_node != nullptr) {
    // draw nodes blend
    glDepthMask(GL_FALSE);
    DrawModelNodes(*model_node, model_transform, Alpha_Blend, settings_->wireframe);
  }

  // FXAA
}

bool ViewerOpenGL::CheckMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform) {
  BoundingBox bbox = mesh.bounding_box.Transform(transform);
  return camera_->GetFrustum().Intersects(bbox);
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

    bool mesh_inside = CheckMeshFrustumCull(mesh, model_matrix);
    if (!mesh_inside) {
      continue;
    }

    if (wireframe) {
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
  uniforms.data.u_modelViewProjectionMatrix = camera_->ProjectionMatrix() * camera_->ViewMatrix() * transform;
  uniforms.data.u_inverseTransposeModelMatrix = glm::mat3(glm::transpose(glm::inverse(transform)));
}

void ViewerOpenGL::UpdateUniformLight(UniformsLight &uniforms) {
  uniforms.data.u_ambientColor = settings_->ambient_color;
  uniforms.data.u_cameraPosition = camera_->Eye();
  uniforms.data.u_pointLightPosition = settings_->light_position;
  uniforms.data.u_pointLightColor = settings_->light_color;

  uniforms.show_light = settings_->show_light;
}

}
}