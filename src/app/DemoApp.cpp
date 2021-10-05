/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


#include "DemoApp.h"
#include "Environment.h"
#include "shader/BlinnPhongShader.h"
#include "shader/PBRShader.h"
#include "shader/SkyboxShader.h"
#include "shader/FXAAShader.h"

#define CREATE_SHADER(PREFIX)                                                   \
  shader_context.vertex_shader = std::make_shared<PREFIX##VertexShader>();      \
  shader_context.fragment_shader = std::make_shared<PREFIX##FragmentShader>();  \
  shader_context.uniforms = std::make_shared<PREFIX##ShaderUniforms>();         \
  shader_context.varyings_size = sizeof(PREFIX##ShaderVaryings);                \


namespace SoftGL {
namespace App {

DemoApp::~DemoApp() {
  if (settingPanel_) {
    settingPanel_->Destroy();
    settingPanel_ = nullptr;
  }
}

bool DemoApp::Create(void *window, int width, int height) {
  width_ = width;
  height_ = height;

  camera_ = std::make_shared<Camera>();
  camera_->SetPerspective(glm::radians(60.f), (float) width / (float) height, 0.01f, 100.0f);
  auto orbitCtrl = std::make_shared<OrbitController>(*camera_);
  orbitController_ = std::make_shared<SmoothOrbitController>(orbitCtrl);
  settings_ = std::make_shared<Settings>();
  settingPanel_ = std::make_shared<SettingPanel>(*settings_);
  settingPanel_->Init(window, width, height);
  model_loader_ = std::make_shared<ModelLoader>();

  settings_->BindModelLoader(*model_loader_);
  settings_->BindOrbitController(*orbitCtrl);

  renderer_ = std::make_shared<Renderer>();
  fxaa_filter_ = std::make_shared<QuadFilter>(width, height);

  // skybox
  model_loader_->LoadSkyBoxTex(settings_->skybox_path);
  return model_loader_->LoadModel(settings_->model_path);
}

void DemoApp::DrawFrame() {
  camera_->Update();
  orbitController_->Update();
  settings_->Update();

  DrawFrameInternal();
}

void DemoApp::DrawFrameInternal() {
  aa_type_ = (AAType) settings_->aa_type;
  if (settings_->wireframe) {
    aa_type_ = AAType_NONE;
  }
  switch (aa_type_) {
    case AAType_FXAA: {
      renderer_->Create(width_, height_, camera_->Near(), camera_->Far());
      fxaa_tex_.buffer = renderer_->GetFrameColor();
      out_color_ = fxaa_filter_->GetFrameColor().get();
    }
      break;
    case AAType_SSAA: {
      renderer_->Create(width_ * 2, height_ * 2, camera_->Near(), camera_->Far());
      out_color_ = renderer_->GetFrameColor().get();
    }
      break;
    default: {
      renderer_->Create(width_, height_, camera_->Near(), camera_->Far());
      out_color_ = renderer_->GetFrameColor().get();
    }
      break;
  }

  renderer_->depth_test = settings_->depth_test;
  renderer_->depth_func = Depth_GREATER;  // Reversed-Z
  renderer_->depth_mask = true;
  renderer_->cull_face_back = settings_->cull_face;
  renderer_->wireframe_show_clip = settings_->wireframe_show_clip;

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
    renderer_->early_z = settings_->early_z;
    DrawModelNodes(*model_node, model_transform, Alpha_Opaque, settings_->wireframe);
    DrawModelNodes(*model_node, model_transform, Alpha_Mask, settings_->wireframe);
    renderer_->early_z = false;
  }

  // skybox
  if (settings_->show_skybox) {
    renderer_->depth_func = Depth_GEQUAL;
    renderer_->depth_mask = false;

    glm::mat4 skybox_matrix(1.f);
    ModelMesh &skybox_node = model_loader_->GetSkyBoxMesh();
    DrawSkybox(skybox_node, skybox_matrix);

    renderer_->depth_func = Depth_GREATER;
    renderer_->depth_mask = true;
  }

  if (model_node != nullptr) {
    // draw nodes blend
    renderer_->depth_mask = false;
    DrawModelNodes(*model_node, model_transform, Alpha_Blend, settings_->wireframe);
  }

  // FXAA
  if (aa_type_ == AAType_FXAA) {
    FXAAPostProcess();
  }
}

bool DemoApp::CheckMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform) {
  BoundingBox bbox = mesh.bounding_box.Transform(transform);
  return camera_->GetFrustum().Intersects(bbox);
}

void DemoApp::DrawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe) {
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
      renderer_->DrawMeshWireframe(mesh, transform);
    } else {
      InitShaderTextured(shader_context, mesh, model_matrix);
      renderer_->DrawMeshTextured(mesh, transform);
    }
  }

  // draw child
  for (auto &child_node : node.children) {
    DrawModelNodes(child_node, model_matrix, mode, wireframe);
  }
}

void DemoApp::DrawSkybox(ModelMesh &mesh, glm::mat4 &transform) {
  auto &shader_context = renderer_->GetShaderContext();
  InitShaderSkybox(shader_context, transform);
  renderer_->DrawMeshTextured(mesh, transform);
}

void DemoApp::DrawWorldAxis(ModelLines &lines, glm::mat4 &transform) {
  auto &shader_context = renderer_->GetShaderContext();
  InitShaderBase(shader_context, transform);
  renderer_->DrawLines(lines, transform);
}

void DemoApp::DrawLights(ModelPoints &points, glm::mat4 &transform) {
  auto &shader_context = renderer_->GetShaderContext();
  InitShaderBase(shader_context, transform);
  renderer_->DrawPoints(points, transform);
}

void DemoApp::FXAAPostProcess() {
  auto &shader_context = fxaa_filter_->GetShaderContext();
  InitShaderFXAA(shader_context);

  auto clear_color = settings_->clear_color;
  fxaa_filter_->Clear(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
  fxaa_filter_->Draw();
}

void DemoApp::InitShaderBase(ShaderContext &shader_context, glm::mat4 &model_matrix) {
  CREATE_SHADER(Base)
  BindShaderUniforms(shader_context, model_matrix);
}

void DemoApp::InitShaderTextured(ShaderContext &shader_context, ModelMesh &mesh, glm::mat4 &model_matrix) {
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

void DemoApp::InitShaderSkybox(ShaderContext &shader_context, glm::mat4 &model_matrix) {
  CREATE_SHADER(Skybox)
  BindShaderUniforms(shader_context, model_matrix);

  // skybox uniforms
  auto *skybox_uniforms_ptr = (SkyboxShaderUniforms *) shader_context.uniforms.get();

  // only rotation
  glm::mat4 view_matrix = glm::mat3(camera_->ViewMatrix());
  skybox_uniforms_ptr->u_modelViewProjectionMatrix = camera_->ProjectionMatrix() * view_matrix * model_matrix;

  auto skyboxTex = model_loader_->GetSkyBoxTexture();
  if (skyboxTex == nullptr) {
    return;
  }
  switch (skyboxTex->type) {
    case Skybox_Cube: {
      for (int i = 0; i < 6; i++) {
        skybox_uniforms_ptr->u_cubeMap.SetTexture(&skyboxTex->cube[i], i);
      }
      skybox_uniforms_ptr->u_cubeMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
      skybox_uniforms_ptr->u_cubeMap.SetFilterMode(Filter_LINEAR);
    }
      break;
    case Skybox_Equirectangular: {
#ifndef SOFTGL_EQUIRECTANGULAR_TO_CUBE
      skybox_uniforms_ptr->u_equirectangularMap.SetTexture(&skyboxTex->equirectangular);
      skybox_uniforms_ptr->u_equirectangularMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
      skybox_uniforms_ptr->u_equirectangularMap.SetFilterMode(Filter_LINEAR);
#else
      if (skyboxTex->cube[0].buffer == nullptr) {
        Environment::ConvertEquirectangular(skyboxTex->equirectangular, skyboxTex->cube);
      }
      for (int i = 0; i < 6; i++) {
        skybox_uniforms_ptr->u_cubeMap.SetTexture(&skyboxTex->cube[i], i);
      }
      skybox_uniforms_ptr->u_cubeMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
      skybox_uniforms_ptr->u_cubeMap.SetFilterMode(Filter_LINEAR);
#endif
    }
      break;
  }
}

void DemoApp::InitShaderFXAA(ShaderContext &shader_context) {
  CREATE_SHADER(FXAA)
  glm::mat4 transform(1.0f);
  BindShaderUniforms(shader_context, transform);

  auto *fxaa_uniforms_ptr = (FXAAShaderUniforms *) shader_context.uniforms.get();
  fxaa_uniforms_ptr->u_screenTexture.SetTexture(&fxaa_tex_);
  fxaa_uniforms_ptr->u_screenTexture.SetWrapMode(Wrap_CLAMP_TO_EDGE);
  fxaa_uniforms_ptr->u_screenTexture.SetFilterMode(Filter_LINEAR);
  fxaa_uniforms_ptr->u_inverseScreenSize = 1.f / glm::vec2(width_, height_);
}

void DemoApp::BindShaderUniforms(ShaderContext &shader_context, glm::mat4 &model_matrix, float alpha_cutoff) {
  shader_context.vertex_shader->uniforms = shader_context.uniforms.get();
  shader_context.fragment_shader->uniforms = shader_context.uniforms.get();

  // base uniforms
  auto *uniforms_ptr = (BaseShaderUniforms *) shader_context.uniforms.get();

  uniforms_ptr->u_modelMatrix = model_matrix;
  uniforms_ptr->u_modelViewProjectionMatrix = camera_->ProjectionMatrix() * camera_->ViewMatrix() * model_matrix;
  uniforms_ptr->u_inverseTransposeModelMatrix = glm::mat3(glm::transpose(glm::inverse(model_matrix)));

  uniforms_ptr->u_cameraPosition = camera_->Eye();
  uniforms_ptr->u_ambientColor = settings_->ambient_color;
  uniforms_ptr->u_showPointLight = settings_->show_light;
  uniforms_ptr->u_pointLightPosition = settings_->light_position;
  uniforms_ptr->u_pointLightColor = settings_->light_color;

  uniforms_ptr->u_alpha_cutoff = alpha_cutoff;
}

void DemoApp::BindShaderTextures(ModelMesh &mesh, std::shared_ptr<BaseShaderUniforms> &uniforms) {
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

      if (settings_->show_skybox) {
        auto skyboxTex = model_loader_->GetSkyBoxTexture();
        if (skyboxTex != nullptr) {
          skyboxTex->InitIBL();

          // irradiance
          for (int i = 0; i < 6; i++) {
            uniforms_ptr->u_irradianceMap.SetTexture(&skyboxTex->irradiance[i], i);
          }
          uniforms_ptr->u_irradianceMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
          uniforms_ptr->u_irradianceMap.SetFilterMode(Filter_LINEAR);

          // prefilter
          for (int i = 0; i < 6; i++) {
            uniforms_ptr->u_prefilterMapMap.SetTexture(&skyboxTex->prefilter[i], i);
          }
          uniforms_ptr->u_prefilterMapMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
          uniforms_ptr->u_prefilterMapMap.SetFilterMode(Filter_LINEAR_MIPMAP_LINEAR);

#ifndef SOFTGL_BRDF_APPROX
          // brdf lut
          uniforms_ptr->u_brdfLutMap.SetTexture(&SkyboxTexture::brdf_lut);
          uniforms_ptr->u_brdfLutMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
          uniforms_ptr->u_brdfLutMap.SetFilterMode(Filter_LINEAR_MIPMAP_LINEAR);
#endif
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

Texture *DemoApp::GetMeshTexture(ModelMesh &mesh, TextureType type) {
  if (mesh.textures.find(type) == mesh.textures.end()) {
    return nullptr;
  }
  return &mesh.textures[type];
}

}
}