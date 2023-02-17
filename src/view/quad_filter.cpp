/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "quad_filter.h"

namespace SoftGL {
namespace View {

QuadFilter::QuadFilter(const std::shared_ptr<Renderer> &renderer,
                       const std::function<bool(ShaderProgram &program)> &shader_func,
                       std::shared_ptr<Texture2D> &tex_in,
                       std::shared_ptr<Texture2D> &tex_out) {
  width_ = tex_out->width;
  height_ = tex_out->height;

  // quad mesh
  quad_mesh_.primitive_type = Primitive_TRIANGLE;
  quad_mesh_.primitive_cnt = 2;
  quad_mesh_.vertexes.push_back({{1.f, -1.f, 0.f}, {1.f, 0.f}});
  quad_mesh_.vertexes.push_back({{-1.f, -1.f, 0.f}, {0.f, 0.f}});
  quad_mesh_.vertexes.push_back({{1.f, 1.f, 0.f}, {1.f, 1.f}});
  quad_mesh_.vertexes.push_back({{-1.f, 1.f, 0.f}, {0.f, 1.f}});
  quad_mesh_.indices = {0, 1, 2, 1, 2, 3};
  quad_mesh_.InitVertexes();

  // renderer
  renderer_ = renderer;

  // fbo
  fbo_ = renderer_->CreateFrameBuffer();
  fbo_->SetColorAttachment(tex_out);

  // vao
  quad_mesh_.vao = renderer_->CreateVertexArrayObject(quad_mesh_);

  // program
  auto program = renderer_->CreateShaderProgram();
  bool success = shader_func(*program);
  if (!success) {
    LOGE("create shader program failed");
    return;
  }
  quad_mesh_.material_textured.shader_program = program;
  quad_mesh_.material_textured.shader_uniforms = std::make_shared<ShaderUniforms>();

  // uniforms
  const char *sampler_name = Material::SamplerName(TextureUsage_QUAD_FILTER);
  auto uniform = renderer_->CreateUniformSampler(sampler_name, tex_in->Type());
  uniform->SetTexture(tex_in);
  quad_mesh_.material_textured.shader_uniforms->samplers[TextureUsage_QUAD_FILTER] = uniform;

  auto uniforms_block = renderer_->CreateUniformBlock("UniformsQuadFilter", sizeof(UniformsQuadFilter));
  UniformsQuadFilter uniforms_filter{glm::vec2(width_, height_)};
  uniforms_block->SetData(&uniforms_filter, sizeof(UniformsQuadFilter));
  quad_mesh_.material_textured.shader_uniforms->blocks[UniformBlock_QuadFilter] = std::move(uniforms_block);

  init_ready_ = true;
}

void QuadFilter::Draw() {
  if (!init_ready_) {
    return;
  }

  renderer_->SetFrameBuffer(*fbo_);
  renderer_->SetViewPort(0, 0, width_, height_);

  renderer_->Clear({});
  renderer_->SetVertexArrayObject(quad_mesh_.vao);
  renderer_->SetRenderState(quad_mesh_.material_textured.render_state);
  renderer_->SetShaderProgram(quad_mesh_.material_textured.shader_program);
  renderer_->SetShaderUniforms(quad_mesh_.material_textured.shader_uniforms);
  renderer_->Draw(quad_mesh_.primitive_type);
}

}
}
