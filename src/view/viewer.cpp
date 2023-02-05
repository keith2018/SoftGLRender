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

void Viewer::Create(int width, int height, int outTexId) {
  width_ = width;
  height_ = height;
  outTexId_ = outTexId;

  // create renderer
  renderer_ = CreateRenderer();

  // create fbo
  fbo_ = renderer_->CreateFrameBuffer();

  // depth attachment
  auto depth_attachment = renderer_->CreateTextureDepth();
  depth_attachment->InitImageData(width, height);
  fbo_->SetDepthAttachment(depth_attachment);

  // color attachment
  Sampler2D sampler;
  sampler.use_mipmaps = false;
  sampler.filter_min = Filter_NEAREST;
  color_tex_out_ = renderer_->CreateTexture2D();
  color_tex_out_->InitImageData(width, height);
  color_tex_out_->SetSampler(sampler);
  fbo_->SetColorAttachment(color_tex_out_, 0);

  if (!fbo_->IsValid()) {
    LOGE("create framebuffer failed");
  }

  uniform_block_scene_ = renderer_->CreateUniformBlock("UniformsScene", sizeof(UniformsScene));
  uniforms_block_mvp_ = renderer_->CreateUniformBlock("UniformsMVP", sizeof(UniformsMVP));
  uniforms_block_color_ = renderer_->CreateUniformBlock("UniformsColor", sizeof(UniformsColor));

  // reset variables
  fxaa_filter_ = nullptr;
  color_tex_fxaa_ = nullptr;
  ibl_placeholder_ = CreateTextureCubeDefault(1, 1);
  program_cache_.clear();
}

void Viewer::DrawFrame(DemoScene &scene) {
  scene_ = &scene;

  // set framebuffer
  renderer_->SetFrameBuffer(*fbo_);

  // anti-aliasing
  auto aa_type = (AAType) config_.aa_type;
  if (config_.wireframe) {
    aa_type = AAType_NONE;
  }

  switch (aa_type) {
    case AAType_FXAA: {
      color_tex_out_->InitImageData(width_, height_);
      FXAASetup();

      fbo_->SetColorAttachment(color_tex_fxaa_, 0);
      fbo_->UpdateAttachmentsSize(width_, height_);
      renderer_->SetViewPort(0, 0, width_, height_);
      break;
    }

    case AAType_SSAA: {
      fbo_->SetColorAttachment(color_tex_out_, 0);
      fbo_->UpdateAttachmentsSize(width_ * 2, height_ * 2);
      renderer_->SetViewPort(0, 0, width_ * 2, height_ * 2);
      break;
    }

    default: {
      fbo_->SetColorAttachment(color_tex_out_, 0);
      fbo_->UpdateAttachmentsSize(width_, height_);
      renderer_->SetViewPort(0, 0, width_, height_);
      break;
    }
  }

  // clear
  ClearState clear_state;
  clear_state.clear_color = config_.clear_color;
  renderer_->Clear(clear_state);

  // update scene uniform
  UpdateUniformScene();

  // draw world axis
  if (config_.world_axis && !config_.show_skybox) {
    glm::mat4 axis_transform(1.0f);
    DrawLines(scene_->world_axis, axis_transform);
  }

  // draw point light
  if (!config_.wireframe && config_.show_light) {
    glm::mat4 light_transform(1.0f);
    DrawPoints(scene_->point_light, light_transform);
  }

  // init skybox ibl
  if (config_.show_skybox) {
    InitSkyboxIBL(scene_->skybox);
  }

  // adjust model center
  glm::mat4 model_transform = AdjustModelCenter(scene_->model->root_aabb);

  // draw model nodes opaque
  ModelNode &model_node = scene_->model->root_node;
  DrawModelNodes(model_node, model_transform, Alpha_Opaque, config_.wireframe);

  // draw skybox
  if (config_.show_skybox) {
    glm::mat4 skybox_transform(1.f);
    DrawSkybox(scene_->skybox, skybox_transform);
  }

  // draw model nodes blend
  DrawModelNodes(model_node, model_transform, Alpha_Blend, config_.wireframe);

  // FXAA
  if (aa_type == AAType_FXAA) {
    FXAADraw();
  }
}

void Viewer::Destroy() {}

void Viewer::FXAASetup() {
  if (!color_tex_fxaa_) {
    Sampler2D sampler;
    sampler.use_mipmaps = false;
    sampler.filter_min = Filter_NEAREST;

    color_tex_fxaa_ = renderer_->CreateTexture2D();
    color_tex_fxaa_->InitImageData(width_, height_);
    color_tex_fxaa_->SetSampler(sampler);
  }

  if (!fxaa_filter_) {
    fxaa_filter_ = std::make_shared<QuadFilter>(CreateRenderer(),
                                                [&](ShaderProgram &program) -> bool {
                                                  return LoadShaders(program, Shading_FXAA);
                                                },
                                                color_tex_fxaa_,
                                                color_tex_out_);
  }
}

void Viewer::FXAADraw() {
  fxaa_filter_->Draw();
}

void Viewer::DrawPoints(ModelPoints &points, glm::mat4 &transform) {
  UpdateUniformMVP(transform);
  UpdateUniformColor(points.material.base_color);

  PipelineSetup(points,
                points.material,
                {uniforms_block_mvp_, uniforms_block_color_},
                false,
                [&](RenderState &rs) -> void {
                  rs.point_size = points.point_size;
                });
  PipelineDraw(points, points.material);
}

void Viewer::DrawLines(ModelLines &lines, glm::mat4 &transform) {
  UpdateUniformMVP(transform);
  UpdateUniformColor(lines.material.base_color);

  PipelineSetup(lines,
                lines.material,
                {uniforms_block_mvp_, uniforms_block_color_},
                false,
                [&](RenderState &rs) -> void {
                  rs.line_width = lines.line_width;
                });
  PipelineDraw(lines, lines.material);
}

void Viewer::DrawSkybox(ModelSkybox &skybox, glm::mat4 &transform) {
  UpdateUniformMVP(transform, true);

  PipelineSetup(skybox,
                skybox.material,
                {uniforms_block_mvp_},
                false,
                [&](RenderState &rs) -> void {
                  rs.depth_func = renderer_->ReverseZ() ? Depth_GREATER : Depth_LEQUAL;
                  rs.depth_mask = false;
                });
  PipelineDraw(skybox, skybox.material);
}

void Viewer::DrawMeshWireframe(ModelMesh &mesh) {
  UpdateUniformColor(glm::vec4(1.f));

  PipelineSetup(mesh,
                mesh.material_wireframe,
                {uniforms_block_mvp_, uniform_block_scene_, uniforms_block_color_},
                false,
                [&](RenderState &rs) -> void {
                  rs.polygon_mode = Polygon_LINE;
                });
  PipelineDraw(mesh, mesh.material_wireframe);
}

void Viewer::DrawMeshTextured(ModelMesh &mesh) {
  UpdateUniformColor(glm::vec4(1.f));

  PipelineSetup(mesh,
                mesh.material_textured,
                {uniforms_block_mvp_, uniform_block_scene_, uniforms_block_color_},
                mesh.material_textured.alpha_mode == Alpha_Blend,
                [&](RenderState &rs) -> void {
                  rs.cull_face = config_.cull_face && (!mesh.material_textured.double_sided);
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
  UpdateUniformMVP(model_matrix);

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
    wireframe ? DrawMeshWireframe(mesh) : DrawMeshTextured(mesh);
  }

  // draw child
  for (auto &child_node : node.children) {
    DrawModelNodes(child_node, model_matrix, mode, wireframe);
  }
}

void Viewer::PipelineSetup(ModelVertexes &vertexes,
                           Material &material,
                           const std::vector<std::shared_ptr<UniformBlock>> &uniform_blocks,
                           bool blend,
                           const std::function<void(RenderState &rs)> &extra_states) {
  SetupVertexArray(vertexes);
  SetupRenderStates(material.render_state, blend, extra_states);
  SetupMaterial(material, uniform_blocks);
}

void Viewer::PipelineDraw(ModelVertexes &vertexes, Material &material) {
  renderer_->SetVertexArrayObject(vertexes.vao);
  renderer_->SetRenderState(material.render_state);
  renderer_->SetShaderProgram(material.shader_program);
  renderer_->SetShaderUniforms(material.shader_uniforms);
  renderer_->Draw(vertexes.primitive_type);
}

void Viewer::SetupVertexArray(ModelVertexes &vertexes) {
  if (!vertexes.vao) {
    vertexes.vao = renderer_->CreateVertexArrayObject(vertexes);
  }
}

void Viewer::SetupRenderStates(RenderState &rs, bool blend, const std::function<void(RenderState &rs)> &extra) const {
  rs.blend = blend;
  rs.blend_src = Factor_SRC_ALPHA;
  rs.blend_dst = Factor_ONE_MINUS_SRC_ALPHA;

  rs.depth_test = config_.depth_test;
  rs.depth_mask = !blend;  // disable depth write
  rs.depth_func = renderer_->ReverseZ() ? Depth_GEQUAL : Depth_LESS;

  rs.cull_face = config_.cull_face;
  rs.polygon_mode = Polygon_FILL;

  rs.line_width = 1.f;
  rs.point_size = 1.f;

  if (extra) {
    extra(rs);
  }
}

void Viewer::SetupTextures(Material &material, std::set<std::string> &shader_defines) {
  SamplerCube sampler_cube;
  Sampler2D sampler_2d;

  for (auto &kv : material.texture_data) {
    std::shared_ptr<Texture> texture = nullptr;
    switch (kv.first) {
      case TextureUsage_IBL_IRRADIANCE:
      case TextureUsage_IBL_PREFILTER: {
        // skip ibl textures
        break;
      }
      case TextureUsage_CUBE: {
        texture = renderer_->CreateTextureCube();
        texture->SetSampler(sampler_cube);
        break;
      }
      default: {
        texture = renderer_->CreateTexture2D();
        texture->SetSampler(sampler_2d);
        break;
      }
    }

    // upload image data
    texture->SetImageData(kv.second);
    material.textures[kv.first] = texture;

    // add shader defines
    const char *sampler_define = Material::SamplerDefine((TextureUsage) kv.first);
    if (sampler_define) {
      shader_defines.insert(sampler_define);
    }
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
      auto uniform = renderer_->CreateUniformSampler(sampler_name);
      uniform->SetTexture(kv.second);
      material.shader_uniforms->samplers[kv.first] = std::move(uniform);
    }
  }
}

bool Viewer::SetupShaderProgram(Material &material, const std::set<std::string> &shader_defines) {
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

void Viewer::SetupMaterial(Material &material, const std::vector<std::shared_ptr<UniformBlock>> &uniform_blocks) {
  std::set<std::string> shader_defines;

  material.CreateTextures([&]() -> void {
    SetupTextures(material, shader_defines);
  });

  material.CreateProgram([&]() -> void {
    if (SetupShaderProgram(material, shader_defines)) {
      SetupSamplerUniforms(material);
      for (auto &block : uniform_blocks) {
        material.shader_uniforms->blocks.emplace_back(block);
      }
    }
  });
}

void Viewer::UpdateUniformScene() {
  uniforms_scene_.u_enablePointLight = config_.show_light ? 1 : 0;
  uniforms_scene_.u_enableIBL = IBLEnabled() ? 1 : 0;

  uniforms_scene_.u_ambientColor = config_.ambient_color;
  uniforms_scene_.u_cameraPosition = camera_.Eye();
  uniforms_scene_.u_pointLightPosition = config_.point_light_position;
  uniforms_scene_.u_pointLightColor = config_.point_light_color;
  uniform_block_scene_->SetData(&uniforms_scene_, sizeof(UniformsScene));
}

void Viewer::UpdateUniformMVP(const glm::mat4 &transform, bool skybox) {
  glm::mat4 view_matrix = camera_.ViewMatrix();
  if (skybox) {
    view_matrix = glm::mat3(view_matrix);  // only rotation
  }
  uniforms_mvp_.u_modelMatrix = transform;
  uniforms_mvp_.u_modelViewProjectionMatrix = camera_.ProjectionMatrix() * view_matrix * transform;
  uniforms_mvp_.u_inverseTransposeModelMatrix = glm::mat3(glm::transpose(glm::inverse(transform)));
  uniforms_block_mvp_->SetData(&uniforms_mvp_, sizeof(UniformsMVP));
}

void Viewer::UpdateUniformColor(const glm::vec4 &color) {
  uniforms_color_.u_baseColor = color;
  uniforms_block_color_->SetData(&uniforms_color_, sizeof(UniformsColor));
}

void Viewer::InitSkyboxIBL(ModelSkybox &skybox) {
  if (skybox.material.ibl_ready) {
    return;
  }

  std::set<std::string> shader_defines;
  skybox.material.CreateTextures([&]() -> void {
    SetupTextures(skybox.material, shader_defines);
  });

  std::shared_ptr<TextureCube> texture_cube = nullptr;

  // convert equirectangular to cube map if needed
  auto tex_cube_it = skybox.material.textures.find(TextureUsage_CUBE);
  if (tex_cube_it == skybox.material.textures.end()) {
    auto tex_eq_it = skybox.material.textures.find(TextureUsage_EQUIRECTANGULAR);
    if (tex_eq_it != skybox.material.textures.end()) {
      auto texture_2d = std::dynamic_pointer_cast<Texture2D>(tex_eq_it->second);
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
        skybox.material.textures[TextureUsage_CUBE] = tex_cvt;

        // update skybox material
        skybox.material.textures.erase(TextureUsage_EQUIRECTANGULAR);
        skybox.material.ResetProgram();
      }
    }
  } else {
    texture_cube = std::dynamic_pointer_cast<TextureCube>(tex_cube_it->second);
  }

  if (!texture_cube) {
    LOGE("InitSkyboxIBL failed: skybox texture cube not available");
    return;
  }

  // generate irradiance map
  auto texture_irradiance = CreateTextureCubeDefault(kIrradianceMapSize, kIrradianceMapSize);
  if (Environment::GenerateIrradianceMap(CreateRenderer(),
                                         [&](ShaderProgram &program) -> bool {
                                           return LoadShaders(program, Shading_IBL_Irradiance);
                                         },
                                         texture_cube,
                                         texture_irradiance)) {
    skybox.material.textures[TextureUsage_IBL_IRRADIANCE] = std::move(texture_irradiance);
  } else {
    LOGE("InitSkyboxIBL failed: generate irradiance map failed");
    return;
  }

  // generate prefilter map
  auto texture_prefilter = CreateTextureCubeDefault(kPrefilterMapSize, kPrefilterMapSize, true);
  if (Environment::GeneratePrefilterMap(CreateRenderer(),
                                        [&](ShaderProgram &program) -> bool {
                                          return LoadShaders(program, Shading_IBL_Prefilter);
                                        },
                                        texture_cube,
                                        texture_prefilter)) {
    skybox.material.textures[TextureUsage_IBL_PREFILTER] = std::move(texture_prefilter);
  } else {
    LOGE("InitSkyboxIBL failed: generate prefilter map failed");
    return;
  }

  skybox.material.ibl_ready = true;
}

bool Viewer::IBLEnabled() {
  return config_.show_skybox && scene_->skybox.material.ibl_ready;
}

void Viewer::UpdateIBLTextures(Material &material) {
  if (!material.shader_uniforms) {
    return;
  }

  auto &samplers = material.shader_uniforms->samplers;
  if (IBLEnabled()) {
    auto &skybox_textures = scene_->skybox.material.textures;
    samplers[TextureUsage_IBL_IRRADIANCE]->SetTexture(skybox_textures[TextureUsage_IBL_IRRADIANCE]);
    samplers[TextureUsage_IBL_PREFILTER]->SetTexture(skybox_textures[TextureUsage_IBL_PREFILTER]);
  } else {
    samplers[TextureUsage_IBL_IRRADIANCE]->SetTexture(ibl_placeholder_);
    samplers[TextureUsage_IBL_PREFILTER]->SetTexture(ibl_placeholder_);
  }
}

std::shared_ptr<TextureCube> Viewer::CreateTextureCubeDefault(int width, int height, bool mipmaps) {
  SamplerCube sampler_cube;
  sampler_cube.use_mipmaps = mipmaps;
  sampler_cube.filter_min = mipmaps ? Filter_LINEAR_MIPMAP_LINEAR : Filter_LINEAR;

  auto texture_cube = renderer_->CreateTextureCube();
  texture_cube->SetSampler(sampler_cube);
  texture_cube->InitImageData(width, height);

  return texture_cube;
}

size_t Viewer::GetShaderProgramCacheKey(ShadingModel shading, const std::set<std::string> &defines) {
  size_t seed = 0;
  HashUtils::HashCombine(seed, (int) shading);
  for (auto &def : defines) {
    HashUtils::HashCombine(seed, def);
  }
  return seed;
}

glm::mat4 Viewer::AdjustModelCenter(BoundingBox &bounds) {
  glm::mat4 model_transform(1.0f);
  glm::vec3 trans = (bounds.max + bounds.min) / -2.f;
  trans.y = -bounds.min.y;
  float bounds_len = glm::length(bounds.max - bounds.min);
  model_transform = glm::scale(model_transform, glm::vec3(3.f / bounds_len));
  model_transform = glm::translate(model_transform, trans);
  return model_transform;
}

bool Viewer::CheckMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform) {
  BoundingBox bbox = mesh.aabb.Transform(transform);
  return camera_.GetFrustum().Intersects(bbox);
}

}
}
