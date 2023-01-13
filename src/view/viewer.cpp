/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "viewer.h"
#include "base/logger.h"
#include "render/opengl/renderer_opengl.h"
#include "shader/opengl/shader_glsl.h"
#include "render/opengl/opengl_utils.h"

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

  common_uniforms.emplace_back(uniform_block_scene_);
  common_uniforms.emplace_back(uniforms_block_mvp_);
  common_uniforms.emplace_back(uniforms_block_color_);
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

  // world axis
  if (config_.world_axis && !config_.show_skybox) {
    auto &lines = scene.world_axis;
    glm::mat4 axis_transform(1.0f);
    DrawLines(lines, axis_transform);
  }

  // point light
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

  // draw model nodes blend
  DrawModelNodes(model_node, model_transform, Alpha_Blend, config_.wireframe);

  // check error
  CheckGLError();
}

void Viewer::Destroy() {

}

void Viewer::DrawPoints(ModelPoints &points, glm::mat4 &transform) {
  // set vertex
  SetupVertexArray(points);

  // set render states
  SetupRenderStates(points.material.render_state);
  points.material.render_state.point_size = points.point_size;

  // set shader program
  if (!points.material.shader_program) {
    SetupUniforms(points.material, {common_uniforms});
    SetupShaderProgram(points.material, Shading_BaseColor);
  }

  // update uniforms
  UpdateUniformMVP(transform);
  UpdateUniformColor(points.material.base_color);

  // draw
  StartRenderPipeline(points, points.material);
}

void Viewer::DrawLines(ModelLines &lines, glm::mat4 &transform) {
  // set vertex
  SetupVertexArray(lines);

  // set render states
  SetupRenderStates(lines.material.render_state);
  lines.material.render_state.line_width = lines.line_width;

  // set shader program
  if (!lines.material.shader_program) {
    SetupUniforms(lines.material, {common_uniforms});
    SetupShaderProgram(lines.material, Shading_BaseColor);
  }

  // update uniforms
  UpdateUniformMVP(transform);
  UpdateUniformColor(lines.material.base_color);

  // draw
  StartRenderPipeline(lines, lines.material);
}

void Viewer::DrawMeshWireframe(ModelMesh &mesh) {
  // set vertex
  SetupVertexArray(mesh);

  // set render states
  SetupRenderStates(mesh.material_wireframe.render_state);
  mesh.material_wireframe.render_state.polygon_mode = LINE;

  // set shader program
  if (!mesh.material_wireframe.shader_program) {
    SetupUniforms(mesh.material_wireframe, {common_uniforms});
    SetupShaderProgram(mesh.material_wireframe, Shading_BaseColor);
  }

  // update uniforms
  UpdateUniformColor(glm::vec4(1.f));

  // draw
  StartRenderPipeline(mesh, mesh.material_wireframe);
}

void Viewer::DrawMeshTextured(ModelMesh &mesh) {
  // set vertex
  SetupVertexArray(mesh);

  // set render states
  bool blend = mesh.material_textured.alpha_mode == Alpha_Blend;
  SetupRenderStates(mesh.material_textured.render_state, blend);
  mesh.material_textured.render_state.polygon_mode = FILL;

  // set shader program
  if (!mesh.material_textured.shader_program) {
    std::vector<std::shared_ptr<Uniform>> sampler_uniforms;
    std::vector<std::string> shader_defines;
    SetupTextures(mesh.material_textured, sampler_uniforms, shader_defines);
    SetupUniforms(mesh.material_textured, {common_uniforms, sampler_uniforms});
    SetupShaderProgram(mesh.material_textured, mesh.material_textured.shading, shader_defines);
  }

  // update uniforms
  UpdateUniformColor(glm::vec4(1.f));

  // draw
  StartRenderPipeline(mesh, mesh.material_textured);
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

void Viewer::SetupVertexArray(VertexArray &vertexes) {
  if (!vertexes.vao) {
    vertexes.vao = renderer_->CreateVertexArrayObject(vertexes);
  }
}

void Viewer::SetupRenderStates(RenderState &rs, bool blend) const {
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
}

void Viewer::SetupUniforms(Material &material, const std::vector<std::vector<std::shared_ptr<Uniform>>> &uniforms) {
  material.uniform_groups = uniforms;
}

void Viewer::SetupTextures(TexturedMaterial &material, std::vector<std::shared_ptr<Uniform>> &sampler_uniforms,
                           std::vector<std::string> &shader_defines) {
  Sampler2D sampler;
  sampler.use_mipmaps = true;
  sampler.wrap_s = Wrap_REPEAT;
  sampler.wrap_t = Wrap_REPEAT;
  sampler.filter_min = Filter_LINEAR_MIPMAP_LINEAR;
  sampler.filter_mag = Filter_LINEAR;

  for (auto &kv : material.texture_data) {
    auto texture = renderer_->CreateTexture2D();
    texture->SetSampler(sampler);
    texture->SetImageData(*kv.second);

    // sampler uniform
    const char *sampler_name = Material::SamplerName((TextureType) kv.first);
    if (sampler_name) {
      auto sampler_uniform = renderer_->CreateUniformSampler(sampler_name);
      sampler_uniform->SetTexture2D(*texture);
      sampler_uniforms.emplace_back(std::move(sampler_uniform));
    }

    // add texture
    material.textures[kv.first] = std::move(texture);

    // shader defines
    const char *sampler_define = Material::SamplerDefine((TextureType) kv.first);
    if (sampler_define) {
      shader_defines.emplace_back(sampler_define);
    }
  }
}

void Viewer::SetupShaderProgram(Material &material, ShadingModel shading_model,
                                const std::vector<std::string> &shader_defines) {
  material.shader_program = renderer_->CreateShaderProgram();
  material.shader_program->AddDefines(shader_defines);

  switch (shading_model) {
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

void Viewer::StartRenderPipeline(VertexArray &vertexes, Material &material) {
  material.shader_program->Use();
  material.BindUniforms();
  renderer_->SetVertexArray(vertexes);
  renderer_->SetRenderState(material.render_state);
  renderer_->Draw(vertexes.primitive_type);
}

void Viewer::UpdateUniformScene() {
  uniforms_scene_.u_enablePointLight = config_.show_light ? 1 : 0;
  uniforms_scene_.u_enableIBL = 0;  // TODO
  uniforms_scene_.u_ambientColor = config_.ambient_color;
  uniforms_scene_.u_cameraPosition = camera_.Eye();
  uniforms_scene_.u_pointLightPosition = config_.point_light_position;
  uniforms_scene_.u_pointLightColor = config_.point_light_color;
  uniform_block_scene_->SetData(&uniforms_scene_, sizeof(UniformsScene));
}

void Viewer::UpdateUniformMVP(const glm::mat4 &transform) {
  uniforms_mvp_.u_modelMatrix = transform;
  uniforms_mvp_.u_modelViewProjectionMatrix = camera_.ProjectionMatrix() * camera_.ViewMatrix() * transform;
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
