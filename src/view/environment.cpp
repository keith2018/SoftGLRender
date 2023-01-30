/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "environment.h"
#include "model_loader.h"
#include "base/logger.h"
#include "render/opengl/renderer_opengl.h"
#include "shader/opengl/shader_glsl.h"


namespace SoftGL {
namespace View {

struct LookAtParam {
  glm::vec3 eye;
  glm::vec3 center;
  glm::vec3 up;
};

struct UniformsPrefilter {
  float u_srcResolution;
  float u_roughness;
};

bool Environment::ConvertEquirectangular(const std::shared_ptr<Texture2D> &tex_in,
                                         std::shared_ptr<TextureCube> &tex_out) {
  CubeRenderContext context;
  bool success = CreateCubeRenderContext(context,
                                         SKYBOX_VS,
                                         SKYBOX_FS,
                                         std::dynamic_pointer_cast<Texture>(tex_in),
                                         TextureUsage_EQUIRECTANGULAR);
  if (!success) {
    LOGE("create render context failed");
    return false;
  }

  DrawCubeFaces(context, tex_out->width, tex_out->height, tex_out);
  return true;
}

bool Environment::GenerateIrradianceMap(const std::shared_ptr<TextureCube> &tex_in,
                                        std::shared_ptr<TextureCube> &tex_out) {
  CubeRenderContext context;
  bool success = CreateCubeRenderContext(context,
                                         IBL_IRRADIANCE_VS,
                                         IBL_IRRADIANCE_FS,
                                         std::dynamic_pointer_cast<Texture>(tex_in),
                                         TextureUsage_CUBE);
  if (!success) {
    LOGE("create render context failed");
    return false;
  }

  DrawCubeFaces(context, tex_out->width, tex_out->height, tex_out);
  return true;
}

bool Environment::GeneratePrefilterMap(const std::shared_ptr<TextureCube> &tex_in,
                                       std::shared_ptr<TextureCube> &tex_out) {
  CubeRenderContext context;
  bool success = CreateCubeRenderContext(context,
                                         IBL_PREFILTER_VS,
                                         IBL_PREFILTER_FS,
                                         std::dynamic_pointer_cast<Texture>(tex_in),
                                         TextureUsage_CUBE);
  if (!success) {
    LOGE("create render context failed");
    return false;
  }

  auto uniforms_block_prefilter = context.renderer->CreateUniformBlock("UniformsPrefilter", sizeof(UniformsPrefilter));
  context.model_skybox.material.shader_program->AddUniformBlock(uniforms_block_prefilter);

  UniformsPrefilter uniforms_prefilter{};

  for (int mip = 0; mip < kPrefilterMaxMipLevels; mip++) {
    int mip_width = (int) (tex_out->width * glm::pow(0.5f, mip));
    int mip_height = (int) (tex_out->height * glm::pow(0.5f, mip));

    DrawCubeFaces(context, mip_width, mip_height, tex_out, mip, [&]() -> void {
      uniforms_prefilter.u_srcResolution = (float) tex_in->width;
      uniforms_prefilter.u_roughness = (float) mip / (float) (kPrefilterMaxMipLevels - 1);
      uniforms_block_prefilter->SetData(&uniforms_prefilter, sizeof(UniformsPrefilter));
    });
  }
  return true;
}

bool Environment::CreateCubeRenderContext(CubeRenderContext &context,
                                          const std::string &vsSource,
                                          const std::string &fsSource,
                                          const std::shared_ptr<Texture> &tex_in,
                                          TextureUsage tex_usage) {
  // camera
  context.camera.SetPerspective(glm::radians(90.f), 1.f, 0.1f, 10.f);

  // model
  ModelLoader::LoadCubeMesh(context.model_skybox);
  context.model_skybox.material.Reset();
  context.model_skybox.material.shading = Shading_Skybox;

  // renderer
  context.renderer = std::make_shared<RendererOpenGL>();

  // fbo
  context.fbo = context.renderer->CreateFrameBuffer();

  // vao
  context.model_skybox.vao = context.renderer->CreateVertexArrayObject(context.model_skybox);

  // shader program
  std::set<std::string> shader_defines = {Material::SamplerDefine(tex_usage)};
  auto program = context.renderer->CreateShaderProgram();
  program->AddDefines(shader_defines);
  bool success = program->CompileAndLink(vsSource, fsSource);
  if (!success) {
    LOGE("create shader program failed");
    return false;
  }
  context.model_skybox.material.shader_program = program;

  // uniforms
  const char *sampler_name = Material::SamplerName(tex_usage);
  auto uniform = context.renderer->CreateUniformSampler(sampler_name);
  uniform->SetTexture(tex_in);
  context.model_skybox.material.shader_program->AddUniformSampler(tex_usage, uniform);

  context.uniforms_block_mvp = context.renderer->CreateUniformBlock("UniformsMVP", sizeof(UniformsMVP));
  context.model_skybox.material.shader_program->AddUniformBlock(context.uniforms_block_mvp);

  return true;
}

void Environment::DrawCubeFaces(CubeRenderContext &context,
                                int width,
                                int height,
                                std::shared_ptr<TextureCube> &tex_out,
                                int tex_out_level,
                                const std::function<void()> &before_draw) {
  static LookAtParam capture_views[] = {
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)}
  };

  UniformsMVP uniforms_mvp{};
  glm::mat4 model_matrix(1.f);

  // draw
  context.renderer->SetFrameBuffer(*context.fbo);
  context.renderer->SetViewPort(0, 0, width, height);

  for (int i = 0; i < 6; i++) {
    auto &param = capture_views[i];
    context.camera.LookAt(param.eye, param.center, param.up);

    // update mvp
    glm::mat4 view_matrix = glm::mat3(context.camera.ViewMatrix());  // only rotation
    uniforms_mvp.u_modelViewProjectionMatrix = context.camera.ProjectionMatrix() * view_matrix * model_matrix;
    context.uniforms_block_mvp->SetData(&uniforms_mvp, sizeof(UniformsMVP));

    if (before_draw) {
      before_draw();
    }

    // draw
    context.fbo->SetColorAttachment(tex_out, CubeMapFace(TEXTURE_CUBE_MAP_POSITIVE_X + i), tex_out_level);
    context.renderer->Clear({});
    context.renderer->SetVertexArray(context.model_skybox);
    context.renderer->SetRenderState(context.model_skybox.material.render_state);
    context.renderer->SetShaderProgram(*context.model_skybox.material.shader_program);
    context.renderer->Draw(context.model_skybox.primitive_type);
  }
}

}
}
