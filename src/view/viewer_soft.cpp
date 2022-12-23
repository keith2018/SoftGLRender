/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


#include "viewer_soft.h"
#include <glad/glad.h>

#include "environment.h"
#include "render/soft/shader/blinn_phong_shader.h"
#include "render/soft/shader/pbr_shader.h"
#include "render/soft/shader/skybox_shader.h"
#include "render/soft/shader/fxaa_shader.h"


#define CREATE_SHADER(PREFIX)                                                   \
  shader_context.vertex_shader = std::make_shared<PREFIX##VertexShader>();      \
  shader_context.fragment_shader = std::make_shared<PREFIX##FragmentShader>();  \
  shader_context.uniforms = std::make_shared<PREFIX##ShaderUniforms>();         \
  shader_context.varyings_size = sizeof(PREFIX##ShaderVaryings);                \


namespace SoftGL {
namespace View {

bool ViewerSoft::Create(void *window, int width, int height, int outTexId) {
  bool success = Viewer::Create(window, width, height, outTexId);
  if (!success) {
    return false;
  }

  renderer_ = std::make_shared<RendererSoft>();
  fxaa_filter_ = std::make_shared<QuadFilter>(width, height);
  return true;
}

void ViewerSoft::DrawFrame() {
  Viewer::DrawFrame();
  DrawFrameInternal();

  // upload buffer to texture
  auto frame = out_color_;
  if (frame) {
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 (int) frame->GetWidth(),
                 (int) frame->GetHeight(),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 frame->GetRawDataPtr());
  }
}

void ViewerSoft::DrawFrameInternal() {
  aa_type_ = (AAType) viewer_->settings_->aa_type;
  if (viewer_->settings_->wireframe) {
    aa_type_ = AAType_NONE;
  }
  switch (aa_type_) {
    case AAType_FXAA: {
      renderer_->Create(width_, height_, viewer_->camera_->Near(), viewer_->camera_->Far());
      fxaa_tex_.buffer = renderer_->GetFrameColor();
      out_color_ = fxaa_filter_->GetFrameColor().get();
    }
      break;
    case AAType_SSAA: {
      renderer_->Create(width_ * 2, height_ * 2, viewer_->camera_->Near(), viewer_->camera_->Far());
      out_color_ = renderer_->GetFrameColor().get();
    }
      break;
    default: {
      renderer_->Create(width_, height_, viewer_->camera_->Near(), viewer_->camera_->Far());
      out_color_ = renderer_->GetFrameColor().get();
    }
      break;
  }

  renderer_->depth_test = viewer_->settings_->depth_test;
  renderer_->depth_func = Depth_GREATER;  // Reversed-Z
  renderer_->depth_mask = true;
  renderer_->cull_face_back = viewer_->settings_->cull_face;
  renderer_->wireframe_show_clip = viewer_->settings_->wireframe_show_clip;

  auto clear_color = viewer_->settings_->clear_color;
  renderer_->Clear(clear_color.r, clear_color.g, clear_color.b, clear_color.a);

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

  // model root
  ModelNode *model_node = viewer_->model_loader_->GetRootNode();
  glm::mat4 model_transform(1.0f);
  if (model_node != nullptr) {
    // adjust model center
    auto bounds = viewer_->model_loader_->GetRootBoundingBox();
    glm::vec3 trans = (bounds->max + bounds->min) / -2.f;
    trans.y = -bounds->min.y;
    float bounds_len = glm::length(bounds->max - bounds->min);
    model_transform = glm::scale(model_transform, glm::vec3(3.f / bounds_len));
    model_transform = glm::translate(model_transform, trans);

    // draw nodes opaque
    renderer_->early_z = viewer_->settings_->early_z;
    DrawModelNodes(*model_node, model_transform, Alpha_Opaque, viewer_->settings_->wireframe);
    DrawModelNodes(*model_node, model_transform, Alpha_Mask, viewer_->settings_->wireframe);
    renderer_->early_z = false;
  }

  // skybox
  if (viewer_->settings_->show_skybox) {
    renderer_->depth_func = Depth_GEQUAL;
    renderer_->depth_mask = false;

    glm::mat4 skybox_matrix(1.f);
    auto skybox_node = viewer_->model_loader_->GetSkyBoxMesh();
    if (skybox_node) {
      DrawSkybox(*skybox_node, skybox_matrix);
    }

    renderer_->depth_func = Depth_GREATER;
    renderer_->depth_mask = true;
  }

  if (model_node != nullptr) {
    // draw nodes blend
    renderer_->depth_mask = false;
    DrawModelNodes(*model_node, model_transform, Alpha_Blend, viewer_->settings_->wireframe);
  }

  // FXAA
  if (aa_type_ == AAType_FXAA) {
    FXAAPostProcess();
  }
}

bool ViewerSoft::CheckMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform) {
  BoundingBox bbox = mesh.bounding_box.Transform(transform);
  return viewer_->camera_->GetFrustum().Intersects(bbox);
}

void ViewerSoft::DrawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe) {
  glm::mat4 model_matrix = transform * node.transform;
  auto &shader_context = renderer_->GetShaderContext();

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
      InitShaderBase(shader_context, model_matrix);
      renderer_->DrawMeshWireframe(mesh);
    } else {
      InitShaderTextured(shader_context, mesh, model_matrix);
      renderer_->DrawMeshTextured(mesh);
    }
  }

  // draw child
  for (auto &child_node : node.children) {
    DrawModelNodes(child_node, model_matrix, mode, wireframe);
  }
}

void ViewerSoft::DrawSkybox(ModelMesh &mesh, glm::mat4 &transform) {
  auto &shader_context = renderer_->GetShaderContext();
  InitShaderSkybox(shader_context, transform);
  renderer_->DrawMeshTextured(mesh);
}

void ViewerSoft::DrawWorldAxis(ModelLines &lines, glm::mat4 &transform) {
  auto &shader_context = renderer_->GetShaderContext();
  InitShaderBase(shader_context, transform);
  renderer_->DrawLines(lines);
}

void ViewerSoft::DrawLights(ModelPoints &points, glm::mat4 &transform) {
  auto &shader_context = renderer_->GetShaderContext();
  InitShaderBase(shader_context, transform);
  renderer_->DrawPoints(points);
}

void ViewerSoft::FXAAPostProcess() {
  auto &shader_context = fxaa_filter_->GetShaderContext();
  InitShaderFXAA(shader_context);

  auto clear_color = viewer_->settings_->clear_color;
  fxaa_filter_->Clear(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
  fxaa_filter_->Draw();
}

void ViewerSoft::InitShaderBase(ShaderContext &shader_context, glm::mat4 &model_matrix) {
  CREATE_SHADER(Base)
  BindShaderUniforms(shader_context, model_matrix);
}

void ViewerSoft::InitShaderTextured(ShaderContext &shader_context, ModelMesh &mesh, glm::mat4 &model_matrix) {
  switch (mesh.shading_type) {
    case ShadingType_PBR_BRDF: {
      CREATE_SHADER(PBR)
    }
      break;
    case ShadingType_BLINN_PHONG:
    default: {
      CREATE_SHADER(BlinnPhong)
    }
      break;
  }

  float alpha_cutoff = -1.f;
  if (mesh.alpha_mode == Alpha_Mask) {
    alpha_cutoff = mesh.alpha_cutoff;
  }
  BindShaderUniforms(shader_context, model_matrix, alpha_cutoff);
  BindShaderTextures(mesh, shader_context.uniforms);
}

void ViewerSoft::InitShaderSkybox(ShaderContext &shader_context, glm::mat4 &model_matrix) {
  CREATE_SHADER(Skybox)
  BindShaderUniforms(shader_context, model_matrix);

  // skybox uniforms
  auto *skybox_uniforms_ptr = (SkyboxShaderUniforms *) shader_context.uniforms.get();

  // only rotation
  glm::mat4 view_matrix = glm::mat3(viewer_->camera_->ViewMatrix());
  skybox_uniforms_ptr->u_modelViewProjectionMatrix = viewer_->camera_->ProjectionMatrix() * view_matrix * model_matrix;

  auto skyboxTex = viewer_->model_loader_->GetSkyBoxTexture();
  if (skyboxTex == nullptr) {
    return;
  }
  switch (skyboxTex->type) {
    case Skybox_Cube: {
      if (skyboxTex->cube_ready) {
        for (int i = 0; i < 6; i++) {
          skybox_uniforms_ptr->u_cubeMap.SetTexture(&skyboxTex->cube[i], i);
        }
        skybox_uniforms_ptr->u_cubeMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
        skybox_uniforms_ptr->u_cubeMap.SetFilterMode(Filter_LINEAR);
      }
    }
      break;
    case Skybox_Equirectangular: {
#ifndef SOFTGL_EQUIRECTANGULAR_TO_CUBE
      if (skyboxTex->equirectangular_ready) {
        skybox_uniforms_ptr->u_equirectangularMap.SetTexture(&skyboxTex->equirectangular);
        skybox_uniforms_ptr->u_equirectangularMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
        skybox_uniforms_ptr->u_equirectangularMap.SetFilterMode(Filter_LINEAR);
      }
#else
      if (!skyboxTex->cube_ready) {
        Environment::ConvertEquirectangular(skyboxTex->equirectangular, skyboxTex->cube);
      }
      if (skyboxTex->cube_ready) {
        for (int i = 0; i < 6; i++) {
          skybox_uniforms_ptr->u_cubeMap.SetTexture(&skyboxTex->cube[i], i);
        }
        skybox_uniforms_ptr->u_cubeMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
        skybox_uniforms_ptr->u_cubeMap.SetFilterMode(Filter_LINEAR);
      }
#endif
    }
      break;
  }
}

void ViewerSoft::InitShaderFXAA(ShaderContext &shader_context) {
  CREATE_SHADER(FXAA)
  glm::mat4 transform(1.0f);
  BindShaderUniforms(shader_context, transform);

  auto *fxaa_uniforms_ptr = (FXAAShaderUniforms *) shader_context.uniforms.get();
  fxaa_uniforms_ptr->u_screenTexture.SetTexture(&fxaa_tex_);
  fxaa_uniforms_ptr->u_screenTexture.SetWrapMode(Wrap_CLAMP_TO_EDGE);
  fxaa_uniforms_ptr->u_screenTexture.SetFilterMode(Filter_LINEAR);
  fxaa_uniforms_ptr->u_inverseScreenSize = 1.f / glm::vec2(width_, height_);
}

void ViewerSoft::BindShaderUniforms(ShaderContext &shader_context, glm::mat4 &model_matrix, float alpha_cutoff) {
  shader_context.vertex_shader->uniforms = shader_context.uniforms.get();
  shader_context.fragment_shader->uniforms = shader_context.uniforms.get();

  // base uniforms
  auto *uniforms_ptr = (BaseShaderUniforms *) shader_context.uniforms.get();

  uniforms_ptr->u_modelMatrix = model_matrix;
  uniforms_ptr->u_modelViewProjectionMatrix =
      viewer_->camera_->ProjectionMatrix() * viewer_->camera_->ViewMatrix() * model_matrix;
  uniforms_ptr->u_inverseTransposeModelMatrix = glm::mat3(glm::transpose(glm::inverse(model_matrix)));

  uniforms_ptr->u_cameraPosition = viewer_->camera_->Eye();
  uniforms_ptr->u_ambientColor = viewer_->settings_->ambient_color;
  uniforms_ptr->u_showPointLight = viewer_->settings_->show_light;
  uniforms_ptr->u_pointLightPosition = viewer_->settings_->light_position;
  uniforms_ptr->u_pointLightColor = viewer_->settings_->light_color;

  uniforms_ptr->u_alpha_cutoff = alpha_cutoff;
}

void ViewerSoft::BindShaderTextures(ModelMesh &mesh, std::shared_ptr<BaseShaderUniforms> &uniforms) {
  std::vector<Sampler2D *> samplers;
  switch (mesh.shading_type) {
    case ShadingType_PBR_BRDF: {
      auto *uniforms_ptr = (PBRShaderUniforms *) uniforms.get();
      uniforms_ptr->u_normalMap.SetTexture(GetMeshTexture(mesh, TextureType_NORMALS));
      uniforms_ptr->u_emissiveMap.SetTexture(GetMeshTexture(mesh, TextureType_EMISSIVE));
      uniforms_ptr->u_albedoMap.SetTexture(GetMeshTexture(mesh, TextureType_PBR_ALBEDO));
      uniforms_ptr->u_metalRoughnessMap.SetTexture(GetMeshTexture(mesh, TextureType_PBR_METAL_ROUGHNESS));
      uniforms_ptr->u_aoMap.SetTexture(GetMeshTexture(mesh, TextureType_PBR_AMBIENT_OCCLUSION));

      samplers.push_back(&uniforms_ptr->u_normalMap);
      samplers.push_back(&uniforms_ptr->u_emissiveMap);
      samplers.push_back(&uniforms_ptr->u_albedoMap);
      samplers.push_back(&uniforms_ptr->u_metalRoughnessMap);
      samplers.push_back(&uniforms_ptr->u_aoMap);

      if (viewer_->settings_->show_skybox) {
        auto skyboxTex = viewer_->model_loader_->GetSkyBoxTexture();
        if (skyboxTex != nullptr) {
          skyboxTex->InitIBL();

          // irradiance
          if (skyboxTex->irradiance_ready) {
            for (int i = 0; i < 6; i++) {
              uniforms_ptr->u_irradianceMap.SetTexture(&skyboxTex->irradiance[i], i);
            }
            uniforms_ptr->u_irradianceMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
            uniforms_ptr->u_irradianceMap.SetFilterMode(Filter_LINEAR);
          }

          // prefilter
          if (skyboxTex->prefilter_ready) {
            for (int i = 0; i < 6; i++) {
              uniforms_ptr->u_prefilterMapMap.SetTexture(&skyboxTex->prefilter[i], i);
            }
            uniforms_ptr->u_prefilterMapMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
            uniforms_ptr->u_prefilterMapMap.SetFilterMode(Filter_LINEAR_MIPMAP_LINEAR);
          }
        }
      }
    }
      break;
    case ShadingType_BLINN_PHONG:
    default: {
      auto *uniforms_ptr = (BlinnPhongShaderUniforms *) uniforms.get();
      uniforms_ptr->u_diffuseMap.SetTexture(GetMeshTexture(mesh, TextureType_DIFFUSE));
      uniforms_ptr->u_normalMap.SetTexture(GetMeshTexture(mesh, TextureType_NORMALS));
      uniforms_ptr->u_emissiveMap.SetTexture(GetMeshTexture(mesh, TextureType_EMISSIVE));
      uniforms_ptr->u_aoMap.SetTexture(GetMeshTexture(mesh, TextureType_PBR_AMBIENT_OCCLUSION));

      samplers.push_back(&uniforms_ptr->u_diffuseMap);
      samplers.push_back(&uniforms_ptr->u_normalMap);
      samplers.push_back(&uniforms_ptr->u_emissiveMap);
      samplers.push_back(&uniforms_ptr->u_aoMap);
    }
      break;
  }

  for (auto sampler : samplers) {
    sampler->SetWrapMode(Wrap_REPEAT);
    sampler->SetFilterMode(Filter_LINEAR);
  }
}

Texture *ViewerSoft::GetMeshTexture(ModelMesh &mesh, TextureType type) {
  if (mesh.textures.find(type) == mesh.textures.end()) {
    return nullptr;
  }
  return &mesh.textures[type];
}

}
}