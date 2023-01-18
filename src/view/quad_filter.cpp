/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "quad_filter.h"
#include "render/opengl/renderer_opengl.h"


namespace SoftGL {
namespace View {

struct UniformsQuadFilter {
  glm::vec2 u_screenSize;
};

QuadFilter::QuadFilter(std::shared_ptr<Texture2D> &tex_in,
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
  renderer_ = std::make_shared<RendererOpenGL>();

  // fbo
  fbo_ = renderer_->CreateFrameBuffer();
  fbo_->SetColorAttachment(tex_out, 0);

  // vao
  quad_mesh_.vao = renderer_->CreateVertexArrayObject(quad_mesh_);

  // texture
  const char *sampler_name = Material::SamplerName(TextureUsage_QUAD_FILTER);
  auto uniform = renderer_->CreateUniformSampler(sampler_name);
  uniform->SetTexture(tex_in);
  quad_mesh_.material_textured.uniform_samplers[TextureUsage_QUAD_FILTER] = uniform;

  // program
  auto program = renderer_->CreateShaderProgram();
  bool success = program->CompileAndLink(vsSource, fsSource);
  if (!success) {
    LOGE("create shader program failed");
    return;
  }
  quad_mesh_.material_textured.shader_program = program;

  // uniforms
  auto uniforms_block_filter = renderer_->CreateUniformBlock("UniformsQuadFilter", sizeof(UniformsQuadFilter));
  UniformsQuadFilter uniforms_filter{glm::vec2(width_, height_)};
  uniforms_block_filter->SetData(&uniforms_filter, sizeof(UniformsQuadFilter));
  quad_mesh_.material_textured.uniform_blocks.emplace_back(uniforms_block_filter);

  init_ready_ = true;
}

void QuadFilter::Draw() {
  if (!init_ready_) {
    return;
  }

  fbo_->Bind();
  renderer_->SetViewPort(0, 0, width_, height_);

  renderer_->Clear({});
  quad_mesh_.material_textured.shader_program->Use();
  quad_mesh_.material_textured.BindUniforms();
  renderer_->SetVertexArray(quad_mesh_);
  renderer_->SetRenderState(quad_mesh_.material_textured.render_state);
  renderer_->Draw(quad_mesh_.primitive_type);
}

}
}
