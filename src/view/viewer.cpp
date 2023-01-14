/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "viewer.h"
#include "base/logger.h"
#include "render/opengl/renderer_opengl.h"
#include "shader/opengl/shader_glsl.h"


namespace SoftGL {
namespace View {

void Viewer::Create(int width, int height, int outTexId) {
  width_ = width;
  height_ = height;
  outTexId_ = outTexId;

  // create renderer
  renderer_ = std::make_shared<RendererOpenGL>();

  // create fbo
  fbo_ = renderer_->CreateFrameBuffer();

  // depth attachment
  auto depth_attachment = renderer_->CreateDepthTexture();
  depth_attachment->InitImageData(width, height);
  fbo_->SetDepthAttachment(depth_attachment);

  // color attachment
  auto color_attachment = renderer_->CreateTexture2DRef(outTexId);
  color_attachment->InitImageData(width, height);
  Sampler2D sampler;
  sampler.filter_min = Filter_NEAREST;
  sampler.filter_mag = Filter_NEAREST;
  color_attachment->SetSampler(sampler);
  fbo_->SetColorAttachment(color_attachment);

  if (!fbo_->IsValid()) {
    LOGE("create framebuffer failed");
  }

  uniform_block_scene_ = renderer_->CreateUniformBlock("UniformsScene", sizeof(UniformsScene));
  uniforms_block_mvp_ = renderer_->CreateUniformBlock("UniformsMVP", sizeof(UniformsMVP));
  uniforms_block_color_ = renderer_->CreateUniformBlock("UniformsColor", sizeof(UniformsColor));
}

void Viewer::DrawFrame(DemoScene &scene) {
  // bind framebuffer
  fbo_->Bind();

  // view port
  renderer_->SetViewPort(0, 0, width_, height_);

  // clear
  clear_state_.color_flag = true;
  clear_state_.depth_flag = true;
  clear_state_.clear_color = config_.clear_color;
  renderer_->Clear(clear_state_);

  // update scene uniform
  UpdateUniformScene();

  // draw world axis
  if (config_.world_axis && !config_.show_skybox) {
    auto &lines = scene.world_axis;
    glm::mat4 axis_transform(1.0f);
    DrawLines(lines, axis_transform);
  }

  // draw point light
  if (!config_.wireframe && config_.show_light) {
    ModelPoints &points = scene.point_light;
    glm::mat4 light_transform(1.0f);
    DrawPoints(points, light_transform);
  }

  // adjust model center
  glm::mat4 model_transform = AdjustModelCenter(scene.model->root_aabb);

  // draw model nodes opaque
  ModelNode &model_node = scene.model->root_node;
  DrawModelNodes(model_node, model_transform, Alpha_Opaque, config_.wireframe);

  // draw skybox
  if (config_.show_skybox) {
    ModelSkybox &skybox = scene.skybox;
    glm::mat4 skybox_transform(1.f);
    DrawSkybox(skybox, skybox_transform);
  }

  // draw model nodes blend
  DrawModelNodes(model_node, model_transform, Alpha_Blend, config_.wireframe);
}

void Viewer::Destroy() {

}

void Viewer::DrawPoints(ModelPoints &points, glm::mat4 &transform) {
  UpdateUniformMVP(transform);
  UpdateUniformColor(points.material.base_color);

  PipelineDraw(points,
               points.material,
               {uniforms_block_mvp_, uniforms_block_color_},
               false,
               [&](RenderState &rs) -> void {
                 rs.point_size = points.point_size;
               });
}

void Viewer::DrawLines(ModelLines &lines, glm::mat4 &transform) {
  UpdateUniformMVP(transform);
  UpdateUniformColor(lines.material.base_color);

  PipelineDraw(lines,
               lines.material,
               {uniforms_block_mvp_, uniforms_block_color_},
               false,
               [&](RenderState &rs) -> void {
                 rs.line_width = lines.line_width;
               });
}

void Viewer::DrawSkybox(ModelSkybox &skybox, glm::mat4 &transform) {
  UpdateUniformMVP(transform, true);

  PipelineDraw(skybox,
               skybox.material,
               {uniforms_block_mvp_},
               false,
               [&](RenderState &rs) -> void {
                 rs.depth_func = Depth_LEQUAL;
                 rs.depth_mask = false;
               });
}

void Viewer::DrawMeshWireframe(ModelMesh &mesh) {
  UpdateUniformColor(glm::vec4(1.f));

  PipelineDraw(mesh,
               mesh.material_wireframe,
               {uniforms_block_mvp_, uniform_block_scene_, uniforms_block_color_},
               false,
               [&](RenderState &rs) -> void {
                 rs.polygon_mode = LINE;
               });
}

void Viewer::DrawMeshTextured(ModelMesh &mesh) {
  UpdateUniformColor(glm::vec4(1.f));

  PipelineDraw(mesh,
               mesh.material_textured,
               {uniforms_block_mvp_, uniform_block_scene_, uniforms_block_color_},
               mesh.material_textured.alpha_mode == Alpha_Blend,
               nullptr);
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

void Viewer::PipelineDraw(VertexArray &vertexes,
                          Material &material,
                          const std::vector<std::shared_ptr<Uniform>> &uniform_blocks,
                          bool blend,
                          const std::function<void(RenderState &rs)> &extra_states) {
  SetupVertexArray(vertexes);
  SetupRenderStates(material.render_state, blend, extra_states);
  SetupMaterial(material, uniform_blocks);
  StartRenderPipeline(vertexes, material);
}

void Viewer::SetupVertexArray(VertexArray &vertexes) {
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
  rs.depth_func = Depth_LESS;

  rs.cull_face = config_.cull_face;
  rs.polygon_mode = FILL;

  rs.line_width = 1.f;
  rs.point_size = 1.f;

  if (extra) {
    extra(rs);
  }
}

void Viewer::SetupTextures(Material &material,
                           std::vector<std::shared_ptr<Uniform>> &sampler_uniforms,
                           std::set<std::string> &shader_defines) {
  SamplerCube sampler_cube;
  Sampler2D sampler_2d;

  for (auto &kv : material.texture_data) {
    std::shared_ptr<Texture> texture = nullptr;
    switch (kv.first) {
      case TextureUsage_CUBE:
      case TextureUsage_IBL_IRRADIANCE:
      case TextureUsage_IBL_PREFILTER: {
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

    // create sampler uniform
    const char *sampler_name = Material::SamplerName((TextureUsage) kv.first);
    if (sampler_name) {
      auto sampler_uniform = renderer_->CreateUniformSampler(sampler_name);
      sampler_uniform->SetTexture(texture);
      sampler_uniforms.emplace_back(std::move(sampler_uniform));
    }

    // add shader defines
    const char *sampler_define = Material::SamplerDefine((TextureUsage) kv.first);
    if (sampler_define) {
      shader_defines.insert(sampler_define);
    }
  }
}

void Viewer::SetupShaderProgram(Material &material, const std::set<std::string> &shader_defines) {
  material.shader_program = renderer_->CreateShaderProgram();
  material.shader_program->AddDefines(shader_defines);

  switch (material.shading) {
    case Shading_BaseColor:
      material.shader_program->CompileAndLink(BASIC_VS, BASIC_FS);
      break;
    case Shading_BlinnPhong:
      material.shader_program->CompileAndLink(BLINN_PHONG_VS, BLINN_PHONG_FS);
      break;
    case Shading_PBR:
      material.shader_program->CompileAndLink(PBR_IBL_VS, PBR_IBL_FS);
      break;
    case Shading_Skybox:
      material.shader_program->CompileAndLink(SKYBOX_VS, SKYBOX_FS);
      break;
  }
}

void Viewer::SetupMaterial(Material &material, const std::vector<std::shared_ptr<Uniform>> &uniform_blocks) {
  material.InitIfDirty([&]() -> void {
    std::vector<std::shared_ptr<Uniform>> uniform_samplers;
    std::set<std::string> shader_defines;
    SetupTextures(material, uniform_samplers, shader_defines);
    material.uniform_groups = {uniform_blocks, uniform_samplers};
    SetupShaderProgram(material, shader_defines);
  });
}

void Viewer::StartRenderPipeline(VertexArray &vertexes, Material &material) {
  material.shader_program->Use();
  material.BindUniforms();
  renderer_->SetVertexArray(vertexes);
  renderer_->SetRenderState(material.render_state);
  renderer_->Draw(vertexes.primitive_type);
}

void Viewer::UpdateUniformScene() {
  uniforms_scene_.u_enablePointLight = config_.show_light ? 1 : 0;
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
