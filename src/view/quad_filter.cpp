/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "quad_filter.h"

#include <utility>
#include "render/opengl/renderer_opengl.h"

namespace SoftGL {
namespace View {

struct UniformsQuadFilter {
  glm::vec2 u_screenSize;
};

QuadFilter::QuadFilter(const std::shared_ptr<Renderer> &renderer,
                       std::shared_ptr<Texture2D> &tex_in,
                       std::shared_ptr<Texture2D> &tex_out,
                       const std::string &vsSource,
                       const std::string &fsSource) {
  width_ = tex_out->width;
  height_ = tex_out->height;

  // quad mesh
  quad_mesh_.primitive_type = Primitive_TRIANGLES;
  quad_mesh_.primitive_cnt = 2;
  quad_mesh_.vertexes.push_back({{1.f, -1.f, 0.f}, {1.f, 0.f}});
  quad_mesh_.vertexes.push_back({{-1.f, -1.f, 0.f}, {0.f, 0.f}});
  quad_mesh_.vertexes.push_back({{1.f, 1.f, 0.f}, {1.f, 1.f}});
  quad_mesh_.vertexes.push_back({{-1.f, 1.f, 0.f}, {0.f, 1.f}});
  quad_mesh_.indices = {0, 1, 2, 1, 2, 3};

  // renderer
  renderer_ = renderer;

  // fbo
  fbo_ = renderer_->CreateFrameBuffer();
  fbo_->SetColorAttachment(tex_out, 0);

  // vao
  quad_mesh_.vao = renderer_->CreateVertexArrayObject(quad_mesh_);

  // program
  auto program = renderer_->CreateShaderProgram();
  bool success = program->CompileAndLink(vsSource, fsSource);
  if (!success) {
    LOGE("create shader program failed");
    return;
  }
  quad_mesh_.material_textured.shader_program = program;
  quad_mesh_.material_textured.shader_uniforms = std::make_shared<ShaderUniforms>();

  // uniforms
  const char *sampler_name = Material::SamplerName(TextureUsage_QUAD_FILTER);
  auto uniform = renderer_->CreateUniformSampler(sampler_name);
  uniform->SetTexture(tex_in);
  quad_mesh_.material_textured.shader_uniforms->samplers[TextureUsage_QUAD_FILTER] = uniform;

  auto uniforms_block_filter = renderer_->CreateUniformBlock("UniformsQuadFilter", sizeof(UniformsQuadFilter));
  UniformsQuadFilter uniforms_filter{glm::vec2(width_, height_)};
  uniforms_block_filter->SetData(&uniforms_filter, sizeof(UniformsQuadFilter));
  quad_mesh_.material_textured.shader_uniforms->blocks.emplace_back(uniforms_block_filter);

  init_ready_ = true;
}

void QuadFilter::Draw() {
  if (!init_ready_) {
    return;
  }

  renderer_->SetFrameBuffer(*fbo_);
  renderer_->SetViewPort(0, 0, width_, height_);

  renderer_->Clear({});
  renderer_->SetVertexArray(quad_mesh_);
  renderer_->SetRenderState(quad_mesh_.material_textured.render_state);
  renderer_->SetShaderProgram(*quad_mesh_.material_textured.shader_program);
  renderer_->SetShaderUniforms(*quad_mesh_.material_textured.shader_uniforms);
  renderer_->Draw(quad_mesh_.primitive_type);
}

}
}
