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
  uniforms_block_mvp_ = renderer_->CreateUniformBlock("UniformsMVP", sizeof(UniformsMVP));
  uniforms_block_color_ = renderer_->CreateUniformBlock("UniformsColor", sizeof(UniformsColor));
  uniforms_block_blinn_phong_ = renderer_->CreateUniformBlock("UniformsBlinnPhong", sizeof(UniformsBlinnPhong));

  // reset variables
  camera_ = &camera_main_;
  fbo_ = nullptr;
  color_tex_out_ = nullptr;
  depth_tex_out_ = nullptr;
  depth_fbo_ = nullptr;
  depth_fbo_tex_ = nullptr;
  fxaa_filter_ = nullptr;
  color_tex_fxaa_ = nullptr;
  ibl_placeholder_ = CreateTextureCubeDefault(1, 1);
  program_cache_.clear();
}

void Viewer::ConfigRenderer() {}

void Viewer::DrawFrame(DemoScene &scene) {
  scene_ = &scene;

  // init frame buffers
  SetupMainBuffers();

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
    renderer_->SetFrameBuffer(depth_fbo_);
    renderer_->SetViewPort(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

    // clear
    ClearState clear_state;
    clear_state.depth_flag = true;
    clear_state.color_flag = false;
    renderer_->Clear(clear_state);

    // set camera
    if (!camera_depth_) {
      camera_depth_ = std::make_shared<Camera>();
      camera_depth_->SetPerspective(glm::radians(CAMERA_FOV),
                                    (float) SHADOW_MAP_WIDTH / (float) SHADOW_MAP_HEIGHT,
                                    CAMERA_NEAR,
                                    CAMERA_FAR);
    }
    camera_ = camera_depth_.get();

    // draw scene
    DrawScene(false);
  }

  // FXAA
  if (config_.aa_type == AAType_FXAA) {
    FXAASetup();
  }

  // set fbo & viewport
  renderer_->SetFrameBuffer(fbo_);
  renderer_->SetViewPort(0, 0, width_, height_);

  // clear
  ClearState clear_state;
  clear_state.clear_color = config_.clear_color;
  renderer_->Clear(clear_state);

  // set camera
  camera_ = &camera_main_;

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
  DrawScene(true);

  // FXAA
  if (config_.aa_type == AAType_FXAA) {
    FXAADraw();
  }
}

void Viewer::Destroy() {}

void Viewer::FXAASetup() {
  if (!color_tex_fxaa_) {
    Sampler2DDesc sampler;
    sampler.use_mipmaps = false;
    sampler.filter_min = Filter_LINEAR;

    color_tex_fxaa_ = renderer_->CreateTexture({TextureType_2D, TextureFormat_RGBA8, false});
    color_tex_fxaa_->InitImageData(width_, height_);
    color_tex_fxaa_->SetSamplerDesc(sampler);
  }

  if (!fxaa_filter_) {
    fxaa_filter_ = std::make_shared<QuadFilter>(CreateRenderer(),
                                                [&](ShaderProgram &program) -> bool {
                                                  return LoadShaders(program, Shading_FXAA);
                                                });
  }

  fbo_->SetColorAttachment(color_tex_fxaa_, 0);
  fxaa_filter_->SetTextures(color_tex_fxaa_, color_tex_out_);
}

void Viewer::FXAADraw() {
  fxaa_filter_->Draw();
}

void Viewer::DrawScene(bool skybox) {
  // draw floor
  if (config_.show_floor) {
    glm::mat4 floor_matrix(1.0f);
    UpdateUniformMVP(floor_matrix, camera_->ViewMatrix(), camera_->ProjectionMatrix());
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
  if (skybox && config_.show_skybox) {
    glm::mat4 skybox_transform(1.f);
    DrawSkybox(scene_->skybox, skybox_transform);
  }

  // draw model nodes blend
  DrawModelNodes(model_node, scene_->model->centered_transform, Alpha_Blend, config_.wireframe);
}

void Viewer::DrawPoints(ModelPoints &points, glm::mat4 &transform) {
  UpdateUniformMVP(transform, camera_->ViewMatrix(), camera_->ProjectionMatrix());
  UpdateUniformColor(points.material.base_color);

  PipelineSetup(points,
                points.material,
                {
                    {UniformBlock_MVP, uniforms_block_mvp_},
                    {UniformBlock_Color, uniforms_block_color_},
                },
                [&](RenderState &rs) -> void {
                  rs.point_size = points.point_size;
                });
  PipelineDraw(points, points.material);
}

void Viewer::DrawLines(ModelLines &lines, glm::mat4 &transform) {
  UpdateUniformMVP(transform, camera_->ViewMatrix(), camera_->ProjectionMatrix());
  UpdateUniformColor(lines.material.base_color);

  PipelineSetup(lines,
                lines.material,
                {
                    {UniformBlock_MVP, uniforms_block_mvp_},
                    {UniformBlock_Color, uniforms_block_color_},
                },
                [&](RenderState &rs) -> void {
                  rs.line_width = lines.line_width;
                });
  PipelineDraw(lines, lines.material);
}

void Viewer::DrawSkybox(ModelSkybox &skybox, glm::mat4 &transform) {
  UpdateUniformMVP(transform, glm::mat3(camera_->ViewMatrix()), camera_->ProjectionMatrix());

  PipelineSetup(skybox,
                *skybox.material,
                {
                    {UniformBlock_MVP, uniforms_block_mvp_}
                },
                [&](RenderState &rs) -> void {
                  rs.depth_func = config_.reverse_z ? DepthFunc_GEQUAL : DepthFunc_LEQUAL;
                  rs.depth_mask = false;
                });
  PipelineDraw(skybox, *skybox.material);
}

void Viewer::DrawMeshBaseColor(ModelMesh &mesh, bool wireframe) {
  UpdateUniformColor(mesh.material_base_color.base_color);

  PipelineSetup(mesh,
                mesh.material_base_color,
                {
                    {UniformBlock_MVP, uniforms_block_mvp_},
                    {UniformBlock_Scene, uniform_block_scene_},
                    {UniformBlock_Color, uniforms_block_color_},
                },
                [&](RenderState &rs) -> void {
                  rs.polygon_mode = wireframe ? PolygonMode_LINE : PolygonMode_FILL;
                });
  PipelineDraw(mesh, mesh.material_base_color);
}

void Viewer::DrawMeshTextured(ModelMesh &mesh, float specular) {
  UpdateUniformColor(glm::vec4(1.f));
  UpdateUniformBlinnPhong(specular);

  PipelineSetup(mesh,
                mesh.material_textured,
                {
                    {UniformBlock_MVP, uniforms_block_mvp_},
                    {UniformBlock_Scene, uniform_block_scene_},
                    {UniformBlock_Color, uniforms_block_color_},
                    {UniformBlock_BlinnPhong, uniforms_block_blinn_phong_},
                });

  // update IBL textures
  if (mesh.material_textured.shading == Shading_PBR) {
    UpdateIBLTextures(mesh.material_textured);
  }

  PipelineDraw(mesh, mesh.material_textured);
}

void Viewer::DrawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe) {
  // update mvp uniform
  glm::mat4 model_matrix = transform * node.transform;
  UpdateUniformMVP(model_matrix, camera_->ViewMatrix(), camera_->ProjectionMatrix());

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

  if (!fbo_) {
    fbo_ = renderer_->CreateFrameBuffer();
  }
  fbo_->SetColorAttachment(color_tex_out_, 0);
  fbo_->SetDepthAttachment(depth_tex_out_);

  if (!fbo_->IsValid()) {
    LOGE("SetupMainBuffers failed");
  }
}

void Viewer::SetupShowMapBuffers() {
  if (!depth_fbo_) {
    depth_fbo_ = renderer_->CreateFrameBuffer();
    if (!depth_fbo_tex_) {
      depth_fbo_tex_ = renderer_->CreateTexture({TextureType_2D, TextureFormat_DEPTH, false});
      depth_fbo_tex_->InitImageData(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
    }
    depth_fbo_->SetDepthAttachment(depth_fbo_tex_);

    if (!depth_fbo_->IsValid()) {
      LOGE("SetupShowMapBuffers failed");
    }
  }
}

void Viewer::SetupMainColorBuffer(bool multi_sample) {
  if (!color_tex_out_ || color_tex_out_->multi_sample != multi_sample) {
    Sampler2DDesc sampler;
    sampler.use_mipmaps = false;
    sampler.filter_min = Filter_LINEAR;
    color_tex_out_ = renderer_->CreateTexture({TextureType_2D, TextureFormat_RGBA8, multi_sample});
    color_tex_out_->InitImageData(width_, height_);
    color_tex_out_->SetSamplerDesc(sampler);
  }
}

void Viewer::SetupMainDepthBuffer(bool multi_sample) {
  if (!depth_tex_out_ || depth_tex_out_->multi_sample != multi_sample) {
    depth_tex_out_ = renderer_->CreateTexture({TextureType_2D, TextureFormat_DEPTH, multi_sample});
    depth_tex_out_->InitImageData(width_, height_);
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
  uniforms_scene_.u_enablePointLight = config_.show_light ? 1 : 0;
  uniforms_scene_.u_enableIBL = IBLEnabled() ? 1 : 0;

  uniforms_scene_.u_ambientColor = config_.ambient_color;
  uniforms_scene_.u_cameraPosition = camera_->Eye();
  uniforms_scene_.u_pointLightPosition = config_.point_light_position;
  uniforms_scene_.u_pointLightColor = config_.point_light_color;
  uniform_block_scene_->SetData(&uniforms_scene_, sizeof(UniformsScene));
}

void Viewer::UpdateUniformMVP(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &proj) {
  uniforms_mvp_.u_modelMatrix = model;
  uniforms_mvp_.u_modelViewProjectionMatrix = proj * view * model;
  uniforms_mvp_.u_inverseTransposeModelMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
  uniforms_block_mvp_->SetData(&uniforms_mvp_, sizeof(UniformsMVP));
}

void Viewer::UpdateUniformColor(const glm::vec4 &color) {
  uniforms_color_.u_baseColor = color;
  uniforms_block_color_->SetData(&uniforms_color_, sizeof(UniformsColor));
}

void Viewer::UpdateUniformBlinnPhong(float specular) {
  uniforms_blinn_phong_.u_kSpecular = specular;
  uniforms_block_blinn_phong_->SetData(&uniforms_blinn_phong_, sizeof(UniformsBlinnPhong));
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

std::shared_ptr<Texture> Viewer::CreateTextureCubeDefault(int width, int height, bool mipmaps) {
  SamplerCubeDesc sampler_cube;
  sampler_cube.use_mipmaps = mipmaps;
  sampler_cube.filter_min = mipmaps ? Filter_LINEAR_MIPMAP_LINEAR : Filter_LINEAR;

  auto texture_cube = renderer_->CreateTexture({TextureType_CUBE, TextureFormat_RGBA8, false});
  texture_cube->SetSamplerDesc(sampler_cube);
  texture_cube->InitImageData(width, height);

  return texture_cube;
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
