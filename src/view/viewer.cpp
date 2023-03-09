/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "viewer.h"
#include <algorithm>
#include "base/logger.h"
#include "base/hash_utils.h"
#include "environment.h"

namespace SoftGL {
namespace View {

#define SHADOW_MAP_WIDTH 512
#define SHADOW_MAP_HEIGHT 512

void Viewer::Create(int width, int height, int outTexId) {
  width_ = width;
  height_ = height;
  outTexId_ = outTexId;

  // create renderer
  renderer_ = CreateRenderer();

  // create uniforms
  uniform_block_scene_ = renderer_->CreateUniformBlock("UniformsScene", sizeof(UniformsScene));
  uniforms_block_model_ = renderer_->CreateUniformBlock("UniformsModel", sizeof(UniformsModel));
  uniforms_block_material_ = renderer_->CreateUniformBlock("UniformsMaterial", sizeof(UniformsMaterial));

  // reset variables
  camera_ = &camera_main_;
  fbo_main_ = nullptr;
  tex_color_main_ = nullptr;
  tex_depth_main_ = nullptr;
  fbo_shadow_ = nullptr;
  tex_depth_shadow_ = nullptr;
  shadow_placeholder_ = CreateTexture2DDefault(1, 1, TextureFormat_DEPTH);
  fxaa_filter_ = nullptr;
  tex_color_fxaa_ = nullptr;
  ibl_placeholder_ = CreateTextureCubeDefault(1, 1);
  program_cache_.clear();
}

void Viewer::ConfigRenderer() {}

void Viewer::DrawFrame(DemoScene &scene) {
  scene_ = &scene;

  // init frame buffers
  SetupMainBuffers();

  // set fbo & viewport
  renderer_->SetFrameBuffer(fbo_main_);
  renderer_->SetViewPort(0, 0, width_, height_);

  // clear
  ClearState clear_state;
  clear_state.clear_color = config_.clear_color;
  renderer_->Clear(clear_state);

  // update scene uniform
  UpdateUniformScene();

  // init skybox ibl
  if (config_.show_skybox && config_.pbr_ibl) {
    InitSkyboxIBL(scene_->skybox);
  }

  // draw shadow map
  if (config_.shadow_map) {
    // set fbo & viewport
    SetupShowMapBuffers();
    renderer_->SetFrameBuffer(fbo_shadow_);
    renderer_->SetViewPort(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

    // clear
    ClearState clear_depth;
    clear_depth.depth_flag = true;
    clear_depth.color_flag = false;
    renderer_->Clear(clear_depth);

    // set camera
    if (!camera_depth_) {
      camera_depth_ = std::make_shared<Camera>();
      camera_depth_->SetPerspective(glm::radians(CAMERA_FOV),
                                    (float) SHADOW_MAP_WIDTH / (float) SHADOW_MAP_HEIGHT,
                                    CAMERA_NEAR,
                                    CAMERA_FAR);
    }
    camera_depth_->LookAt(config_.point_light_position, glm::vec3(0), glm::vec3(0, 1, 0));
    camera_depth_->Update();
    camera_ = camera_depth_.get();

    // draw scene
    DrawScene(false);

    // set back to main camera
    camera_ = &camera_main_;

    // set back to main fbo
    renderer_->SetFrameBuffer(fbo_main_);
    renderer_->SetViewPort(0, 0, width_, height_);
  }

  // FXAA
  if (config_.aa_type == AAType_FXAA) {
    FXAASetup();
  }

  // draw point light
  if (config_.show_light) {
    glm::mat4 light_transform(1.0f);
    DrawPoints(scene_->point_light, light_transform);
  }

  // draw world axis
  if (config_.world_axis) {
    glm::mat4 axis_transform(1.0f);
    DrawLines(scene_->world_axis, axis_transform);
  }

  // draw scene
  DrawScene(config_.show_skybox);

  // FXAA
  if (config_.aa_type == AAType_FXAA) {
    FXAADraw();
  }
}

void Viewer::Destroy() {}

void Viewer::FXAASetup() {
  if (!tex_color_fxaa_) {
    Sampler2DDesc sampler;
    sampler.use_mipmaps = false;
    sampler.filter_min = Filter_LINEAR;

    tex_color_fxaa_ = renderer_->CreateTexture({TextureType_2D, TextureFormat_RGBA8, false});
    tex_color_fxaa_->SetSamplerDesc(sampler);
    tex_color_fxaa_->InitImageData(width_, height_);
  }

  if (!fxaa_filter_) {
    fxaa_filter_ = std::make_shared<QuadFilter>(CreateRenderer(),
                                                [&](ShaderProgram &program) -> bool {
                                                  return LoadShaders(program, Shading_FXAA);
                                                });
  }

  fbo_main_->SetColorAttachment(tex_color_fxaa_, 0);
  fxaa_filter_->SetTextures(tex_color_fxaa_, tex_color_main_);
}

void Viewer::FXAADraw() {
  fxaa_filter_->Draw();
}

void Viewer::DrawScene(bool skybox) {
  // draw floor
  if (config_.show_floor) {
    glm::mat4 floor_matrix(1.0f);
    UpdateUniformModel(floor_matrix, camera_->ViewMatrix(), camera_->ProjectionMatrix());
    if (config_.wireframe) {
      DrawMeshBaseColor(scene_->floor, true);
    } else {
      DrawMeshTextured(scene_->floor, 0.f);
    }
  }

  // draw model nodes opaque
  ModelNode &model_node = scene_->model->root_node;
  DrawModelNodes(model_node, scene_->model->centered_transform, Alpha_Opaque, config_.wireframe);

  // draw skybox
  if (skybox) {
    glm::mat4 skybox_transform(1.f);
    DrawSkybox(scene_->skybox, skybox_transform);
  }

  // draw model nodes blend
  DrawModelNodes(model_node, scene_->model->centered_transform, Alpha_Blend, config_.wireframe);
}

void Viewer::DrawPoints(ModelPoints &points, glm::mat4 &transform) {
  UpdateUniformModel(transform, camera_->ViewMatrix(), camera_->ProjectionMatrix());
  UpdateUniformMaterial(points.material.base_color);

  PipelineSetup(points,
                points.material,
                {
                    {UniformBlock_Model, uniforms_block_model_},
                    {UniformBlock_Material, uniforms_block_material_},
                },
                [&](RenderState &rs) -> void {
                  rs.point_size = points.point_size;
                });
  PipelineDraw(points, points.material);
}

void Viewer::DrawLines(ModelLines &lines, glm::mat4 &transform) {
  UpdateUniformModel(transform, camera_->ViewMatrix(), camera_->ProjectionMatrix());
  UpdateUniformMaterial(lines.material.base_color);

  PipelineSetup(lines,
                lines.material,
                {
                    {UniformBlock_Model, uniforms_block_model_},
                    {UniformBlock_Material, uniforms_block_material_},
                },
                [&](RenderState &rs) -> void {
                  rs.line_width = lines.line_width;
                });
  PipelineDraw(lines, lines.material);
}

void Viewer::DrawSkybox(ModelSkybox &skybox, glm::mat4 &transform) {
  UpdateUniformModel(transform, glm::mat3(camera_->ViewMatrix()), camera_->ProjectionMatrix());

  PipelineSetup(skybox,
                *skybox.material,
                {
                    {UniformBlock_Model, uniforms_block_model_}
                },
                [&](RenderState &rs) -> void {
                  rs.depth_func = config_.reverse_z ? DepthFunc_GEQUAL : DepthFunc_LEQUAL;
                  rs.depth_mask = false;
                });
  PipelineDraw(skybox, *skybox.material);
}

void Viewer::DrawMeshBaseColor(ModelMesh &mesh, bool wireframe) {
  UpdateUniformMaterial(mesh.material_base_color.base_color);

  PipelineSetup(mesh,
                mesh.material_base_color,
                {
                    {UniformBlock_Model, uniforms_block_model_},
                    {UniformBlock_Scene, uniform_block_scene_},
                    {UniformBlock_Material, uniforms_block_material_},
                },
                [&](RenderState &rs) -> void {
                  rs.polygon_mode = wireframe ? PolygonMode_LINE : PolygonMode_FILL;
                });
  PipelineDraw(mesh, mesh.material_base_color);
}

void Viewer::DrawMeshTextured(ModelMesh &mesh, float specular) {
  UpdateUniformMaterial(glm::vec4(1.f), specular);

  PipelineSetup(mesh,
                mesh.material_textured,
                {
                    {UniformBlock_Model, uniforms_block_model_},
                    {UniformBlock_Scene, uniform_block_scene_},
                    {UniformBlock_Material, uniforms_block_material_},
                });

  // update IBL textures
  if (mesh.material_textured.shading == Shading_PBR) {
    UpdateIBLTextures(mesh.material_textured);
  }

  // update shadow textures
  if (config_.shadow_map) {
    UpdateShadowTextures(mesh.material_textured);
  }

  PipelineDraw(mesh, mesh.material_textured);
}

void Viewer::DrawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe) {
  // update mvp uniform
  glm::mat4 model_matrix = transform * node.transform;
  UpdateUniformModel(model_matrix, camera_->ViewMatrix(), camera_->ProjectionMatrix());

  // draw nodes
  for (auto &mesh : node.meshes) {
    if (mesh.material_textured.alpha_mode != mode) {
      continue;
    }

    // frustum cull
    bool mesh_inside = CheckMeshFrustumCull(mesh, model_matrix);
    if (!mesh_inside) {
      continue;
    }

    // draw mesh
    wireframe ? DrawMeshBaseColor(mesh, true) : DrawMeshTextured(mesh, 1.f);
  }

  // draw child
  for (auto &child_node : node.children) {
    DrawModelNodes(child_node, model_matrix, mode, wireframe);
  }
}

void Viewer::PipelineSetup(ModelVertexes &vertexes,
                           Material &material,
                           const std::unordered_map<int, std::shared_ptr<UniformBlock>> &uniform_blocks,
                           const std::function<void(RenderState &rs)> &extra_states) {
  SetupVertexArray(vertexes);
  SetupRenderStates(material.render_state, material.alpha_mode == Alpha_Blend);
  material.render_state.cull_face = config_.cull_face && (!material.double_sided);
  if (extra_states) {
    extra_states(material.render_state);
  }
  SetupMaterial(material, uniform_blocks);
}

void Viewer::PipelineDraw(ModelVertexes &vertexes, Material &material) {
  renderer_->SetVertexArrayObject(vertexes.vao);
  renderer_->SetRenderState(material.render_state);
  renderer_->SetShaderProgram(material.shader_program);
  renderer_->SetShaderUniforms(material.shader_uniforms);
  renderer_->Draw(vertexes.primitive_type);
}

void Viewer::SetupMainBuffers() {
  if (config_.aa_type == AAType_MSAA) {
    SetupMainColorBuffer(true);
    SetupMainDepthBuffer(true);
  } else {
    SetupMainColorBuffer(false);
    SetupMainDepthBuffer(false);
  }

  if (!fbo_main_) {
    fbo_main_ = renderer_->CreateFrameBuffer();
  }
  fbo_main_->SetColorAttachment(tex_color_main_, 0);
  fbo_main_->SetDepthAttachment(tex_depth_main_);

  if (!fbo_main_->IsValid()) {
    LOGE("SetupMainBuffers failed");
  }
}

void Viewer::SetupShowMapBuffers() {
  if (!fbo_shadow_) {
    fbo_shadow_ = renderer_->CreateFrameBuffer();
    if (!tex_depth_shadow_) {
      Sampler2DDesc sampler;
      sampler.use_mipmaps = false;
      sampler.filter_min = Filter_NEAREST;
      sampler.filter_mag = Filter_NEAREST;
      sampler.wrap_s = Wrap_CLAMP_TO_BORDER;
      sampler.wrap_t = Wrap_CLAMP_TO_BORDER;
      sampler.border_color = glm::vec4(1.f, 1.f, 1.f, 1.f);
      tex_depth_shadow_ = renderer_->CreateTexture({TextureType_2D, TextureFormat_DEPTH, false});
      tex_depth_shadow_->SetSamplerDesc(sampler);
      tex_depth_shadow_->InitImageData(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
    }
    fbo_shadow_->SetDepthAttachment(tex_depth_shadow_);

    if (!fbo_shadow_->IsValid()) {
      LOGE("SetupShowMapBuffers failed");
    }
  }
}

void Viewer::SetupMainColorBuffer(bool multi_sample) {
  if (!tex_color_main_ || tex_color_main_->multi_sample != multi_sample) {
    Sampler2DDesc sampler;
    sampler.use_mipmaps = false;
    sampler.filter_min = Filter_LINEAR;
    tex_color_main_ = renderer_->CreateTexture({TextureType_2D, TextureFormat_RGBA8, multi_sample});
    tex_color_main_->SetSamplerDesc(sampler);
    tex_color_main_->InitImageData(width_, height_);
  }
}

void Viewer::SetupMainDepthBuffer(bool multi_sample) {
  if (!tex_depth_main_ || tex_depth_main_->multi_sample != multi_sample) {
    Sampler2DDesc sampler;
    sampler.use_mipmaps = false;
    sampler.filter_min = Filter_NEAREST;
    sampler.filter_mag = Filter_NEAREST;
    tex_depth_main_ = renderer_->CreateTexture({TextureType_2D, TextureFormat_DEPTH, multi_sample});
    tex_depth_main_->SetSamplerDesc(sampler);
    tex_depth_main_->InitImageData(width_, height_);
  }
}

void Viewer::SetupVertexArray(ModelVertexes &vertexes) {
  if (!vertexes.vao) {
    vertexes.vao = renderer_->CreateVertexArrayObject(vertexes);
  }
}

void Viewer::SetupRenderStates(RenderState &rs, bool blend) const {
  rs.blend = blend;
  rs.blend_parameters.SetBlendFactor(BlendFactor_SRC_ALPHA, BlendFactor_ONE_MINUS_SRC_ALPHA);

  rs.depth_test = config_.depth_test;
  rs.depth_mask = !blend;  // disable depth write
  rs.depth_func = config_.reverse_z ? DepthFunc_GEQUAL : DepthFunc_LESS;

  rs.cull_face = config_.cull_face;
  rs.polygon_mode = PolygonMode_FILL;

  rs.line_width = 1.f;
  rs.point_size = 1.f;
}

void Viewer::SetupTextures(Material &material) {
  for (auto &kv : material.texture_data) {
    std::shared_ptr<Texture> texture = nullptr;
    switch (kv.first) {
      case TextureUsage_IBL_IRRADIANCE:
      case TextureUsage_IBL_PREFILTER: {
        // skip ibl textures
        break;
      }
      case TextureUsage_CUBE: {
        texture = renderer_->CreateTexture({TextureType_CUBE, TextureFormat_RGBA8, false});

        SamplerCubeDesc sampler_cube;
        sampler_cube.wrap_s = kv.second.wrap_mode;
        sampler_cube.wrap_t = kv.second.wrap_mode;
        sampler_cube.wrap_r = kv.second.wrap_mode;
        texture->SetSamplerDesc(sampler_cube);
        break;
      }
      default: {
        texture = renderer_->CreateTexture({TextureType_2D, TextureFormat_RGBA8, false});

        Sampler2DDesc sampler_2d;
        sampler_2d.wrap_s = kv.second.wrap_mode;
        sampler_2d.wrap_t = kv.second.wrap_mode;
        if (config_.mipmaps) {
          sampler_2d.use_mipmaps = true;
          sampler_2d.filter_min = Filter_LINEAR_MIPMAP_LINEAR;
        }
        texture->SetSamplerDesc(sampler_2d);
        break;
      }
    }

    // upload image data
    texture->SetImageData(kv.second.data);
    material.textures[kv.first] = texture;
  }

  // default shadow depth texture
  material.textures[TextureUsage_SHADOWMAP] = shadow_placeholder_;

  // default IBL texture
  if (material.shading == Shading_PBR) {
    material.textures[TextureUsage_IBL_IRRADIANCE] = ibl_placeholder_;
    material.textures[TextureUsage_IBL_PREFILTER] = ibl_placeholder_;
  }
}

void Viewer::SetupSamplerUniforms(Material &material) {
  for (auto &kv : material.textures) {
    // create sampler uniform
    const char *sampler_name = Material::SamplerName((TextureUsage) kv.first);
    if (sampler_name) {
      auto uniform = renderer_->CreateUniformSampler(sampler_name, kv.second->type, kv.second->format);
      uniform->SetTexture(kv.second);
      material.shader_uniforms->samplers[kv.first] = std::move(uniform);
    }
  }
}

bool Viewer::SetupShaderProgram(Material &material) {
  auto shader_defines = GenerateShaderDefines(material);
  size_t cache_key = GetShaderProgramCacheKey(material.shading, shader_defines);

  // try cache
  auto cached_program = program_cache_.find(cache_key);
  if (cached_program != program_cache_.end()) {
    material.shader_program = cached_program->second;
    material.shader_uniforms = std::make_shared<ShaderUniforms>();
    return true;
  }

  auto program = renderer_->CreateShaderProgram();
  program->AddDefines(shader_defines);

  bool success = LoadShaders(*program, material.shading);
  if (success) {
    // add to cache
    program_cache_[cache_key] = program;
    material.shader_program = program;
    material.shader_uniforms = std::make_shared<ShaderUniforms>();
  } else {
    LOGE("SetupShaderProgram failed: %s", Material::ShadingModelStr(material.shading));
  }

  return success;
}

void Viewer::SetupMaterial(Material &material,
                           const std::unordered_map<int, std::shared_ptr<UniformBlock>> &uniform_blocks) {
  material.CreateTextures([&]() -> void {
    SetupTextures(material);
  });

  material.CreateProgram([&]() -> void {
    if (SetupShaderProgram(material)) {
      SetupSamplerUniforms(material);
      for (auto &kv : uniform_blocks) {
        material.shader_uniforms->blocks.insert(kv);
      }
    }
  });
}

void Viewer::UpdateUniformScene() {
  uniforms_scene_.u_ambientColor = config_.ambient_color;
  uniforms_scene_.u_cameraPosition = camera_->Eye();
  uniforms_scene_.u_pointLightPosition = config_.point_light_position;
  uniforms_scene_.u_pointLightColor = config_.point_light_color;
  uniform_block_scene_->SetData(&uniforms_scene_, sizeof(UniformsScene));
}

void Viewer::UpdateUniformModel(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &proj) {
  uniforms_model_.u_modelMatrix = model;
  uniforms_model_.u_modelViewProjectionMatrix = proj * view * model;
  uniforms_model_.u_inverseTransposeModelMatrix = glm::mat3(glm::transpose(glm::inverse(model)));

  // shadow mvp
  if (config_.shadow_map && camera_depth_) {
    uniforms_model_.u_shadowMVPMatrix = camera_depth_->ProjectionMatrix() * camera_depth_->ViewMatrix() * model;
  }

  uniforms_block_model_->SetData(&uniforms_model_, sizeof(UniformsModel));
}

void Viewer::UpdateUniformMaterial(const glm::vec4 &color, float specular) {
  uniforms_material_.u_enableLight = config_.show_light ? 1 : 0;
  uniforms_material_.u_enableIBL = IBLEnabled() ? 1 : 0;
  uniforms_material_.u_enableShadow = config_.shadow_map;

  uniforms_material_.u_kSpecular = specular;
  uniforms_material_.u_baseColor = color;

  uniforms_block_material_->SetData(&uniforms_material_, sizeof(UniformsMaterial));
}

bool Viewer::InitSkyboxIBL(ModelSkybox &skybox) {
  if (skybox.material->ibl_ready) {
    return true;
  }

  if (skybox.material->ibl_error) {
    return false;
  }

  skybox.material->CreateTextures([&]() -> void {
    SetupTextures(*skybox.material);
  });

  std::shared_ptr<Texture> texture_cube = nullptr;

  // convert equirectangular to cube map if needed
  auto tex_cube_it = skybox.material->textures.find(TextureUsage_CUBE);
  if (tex_cube_it == skybox.material->textures.end()) {
    auto tex_eq_it = skybox.material->textures.find(TextureUsage_EQUIRECTANGULAR);
    if (tex_eq_it != skybox.material->textures.end()) {
      auto texture_2d = std::dynamic_pointer_cast<Texture>(tex_eq_it->second);
      auto cube_size = std::min(texture_2d->width, texture_2d->height);
      auto tex_cvt = CreateTextureCubeDefault(cube_size, cube_size);
      auto success = Environment::ConvertEquirectangular(CreateRenderer(),
                                                         [&](ShaderProgram &program) -> bool {
                                                           return LoadShaders(program, Shading_Skybox);
                                                         },
                                                         texture_2d,
                                                         tex_cvt);
      if (success) {
        texture_cube = tex_cvt;
        skybox.material->textures[TextureUsage_CUBE] = tex_cvt;

        // update skybox material
        skybox.material->textures.erase(TextureUsage_EQUIRECTANGULAR);
        skybox.material->ResetProgram();
      }
    }
  } else {
    texture_cube = std::dynamic_pointer_cast<Texture>(tex_cube_it->second);
  }

  if (!texture_cube) {
    LOGE("InitSkyboxIBL failed: skybox texture cube not available");
    skybox.material->ibl_error = true;
    return false;
  }

  // generate irradiance map
  LOGD("generate ibl irradiance map ...");
  auto texture_irradiance = CreateTextureCubeDefault(kIrradianceMapSize, kIrradianceMapSize);
  if (Environment::GenerateIrradianceMap(CreateRenderer(),
                                         [&](ShaderProgram &program) -> bool {
                                           return LoadShaders(program, Shading_IBL_Irradiance);
                                         },
                                         texture_cube,
                                         texture_irradiance)) {
    skybox.material->textures[TextureUsage_IBL_IRRADIANCE] = std::move(texture_irradiance);
  } else {
    LOGE("InitSkyboxIBL failed: generate irradiance map failed");
    skybox.material->ibl_error = true;
    return false;
  }
  LOGD("generate ibl irradiance map done.");

  // generate prefilter map
  LOGD("generate ibl prefilter map ...");
  auto texture_prefilter = CreateTextureCubeDefault(kPrefilterMapSize, kPrefilterMapSize, true);
  if (Environment::GeneratePrefilterMap(CreateRenderer(),
                                        [&](ShaderProgram &program) -> bool {
                                          return LoadShaders(program, Shading_IBL_Prefilter);
                                        },
                                        texture_cube,
                                        texture_prefilter)) {
    skybox.material->textures[TextureUsage_IBL_PREFILTER] = std::move(texture_prefilter);
  } else {
    LOGE("InitSkyboxIBL failed: generate prefilter map failed");
    skybox.material->ibl_error = true;
    return false;
  }
  LOGD("generate ibl prefilter map done.");

  skybox.material->ibl_ready = true;
  return true;
}

bool Viewer::IBLEnabled() {
  return config_.show_skybox && config_.pbr_ibl && scene_->skybox.material->ibl_ready;
}

void Viewer::UpdateIBLTextures(Material &material) {
  if (!material.shader_uniforms) {
    return;
  }

  auto &samplers = material.shader_uniforms->samplers;
  if (IBLEnabled()) {
    auto &skybox_textures = scene_->skybox.material->textures;
    samplers[TextureUsage_IBL_IRRADIANCE]->SetTexture(skybox_textures[TextureUsage_IBL_IRRADIANCE]);
    samplers[TextureUsage_IBL_PREFILTER]->SetTexture(skybox_textures[TextureUsage_IBL_PREFILTER]);
  } else {
    samplers[TextureUsage_IBL_IRRADIANCE]->SetTexture(ibl_placeholder_);
    samplers[TextureUsage_IBL_PREFILTER]->SetTexture(ibl_placeholder_);
  }
}

void Viewer::UpdateShadowTextures(Material &material) {
  if (!material.shader_uniforms) {
    return;
  }

  auto &samplers = material.shader_uniforms->samplers;
  if (config_.shadow_map) {
    samplers[TextureUsage_SHADOWMAP]->SetTexture(tex_depth_shadow_);
  } else {
    samplers[TextureUsage_SHADOWMAP]->SetTexture(shadow_placeholder_);
  }
}

std::shared_ptr<Texture> Viewer::CreateTextureCubeDefault(int width, int height, bool mipmaps) {
  SamplerCubeDesc sampler_cube;
  sampler_cube.use_mipmaps = mipmaps;
  sampler_cube.filter_min = mipmaps ? Filter_LINEAR_MIPMAP_LINEAR : Filter_LINEAR;

  auto texture_cube = renderer_->CreateTexture({TextureType_CUBE, TextureFormat_RGBA8, false});
  texture_cube->SetSamplerDesc(sampler_cube);
  texture_cube->InitImageData(width, height);

  return texture_cube;
}

std::shared_ptr<Texture> Viewer::CreateTexture2DDefault(int width, int height, TextureFormat format, bool mipmaps) {
  Sampler2DDesc sampler_2d;
  sampler_2d.use_mipmaps = mipmaps;
  sampler_2d.filter_min = mipmaps ? Filter_LINEAR_MIPMAP_LINEAR : Filter_LINEAR;

  auto texture_2d = renderer_->CreateTexture({TextureType_2D, format, false});
  texture_2d->SetSamplerDesc(sampler_2d);
  texture_2d->InitImageData(width, height);

  return texture_2d;
}

std::set<std::string> Viewer::GenerateShaderDefines(Material &material) {
  std::set<std::string> shader_defines;
  for (auto &kv : material.textures) {
    const char *sampler_define = Material::SamplerDefine((TextureUsage) kv.first);
    if (sampler_define) {
      shader_defines.insert(sampler_define);
    }
  }
  return shader_defines;
}

size_t Viewer::GetShaderProgramCacheKey(ShadingModel shading, const std::set<std::string> &defines) {
  size_t seed = 0;
  HashUtils::HashCombine(seed, (int) shading);
  for (auto &def : defines) {
    HashUtils::HashCombine(seed, def);
  }
  return seed;
}

bool Viewer::CheckMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform) {
  BoundingBox bbox = mesh.aabb.Transform(transform);
  return camera_->GetFrustum().Intersects(bbox);
}

}
}
