/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "environment.h"
#include "camera.h"
#include "model.h"
#include "model_loader.h"
#include "base/logger.h"
#include "render/renderer.h"
#include "render/framebuffer.h"
#include "render/opengl/renderer_opengl.h"
#include "shader/opengl/shader_glsl.h"


namespace SoftGL {
namespace View {

#define IrradianceMapSize 32
#define PrefilterMaxMipLevels 5
#define PrefilterMapSize 128

struct LookAtParam {
  glm::vec3 eye;
  glm::vec3 center;
  glm::vec3 up;
};

bool Environment::ConvertEquirectangular(std::shared_ptr<Texture2D> &tex_in,
                                         std::shared_ptr<TextureCube> &tex_out) {
  Camera camera;
  camera.SetPerspective(glm::radians(90.f), 1.f, 0.1f, 10.f);

  LookAtParam captureViews[] = {
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)}
  };

  ModelSkybox model_skybox;
  ModelLoader::LoadCubeMesh(model_skybox);
  model_skybox.material.Reset();
  model_skybox.material.shading = Shading_Skybox;
  model_skybox.material.skybox_type = Skybox_Equirectangular;

  // renderer
  std::shared_ptr<Renderer> renderer = std::make_shared<RendererOpenGL>();

  // vao
  model_skybox.vao = renderer->CreateVertexArrayObject(model_skybox);

  // texture
  const char *sampler_name = Material::SamplerName(TextureUsage_EQUIRECTANGULAR);
  auto uniform = renderer->CreateUniformSampler(sampler_name);
  uniform->SetTexture(tex_in);
  std::vector<std::shared_ptr<Uniform>> sampler_uniforms = {uniform};

  // shader program
  std::set<std::string> shader_defines = {Material::SamplerDefine(TextureUsage_EQUIRECTANGULAR)};
  auto program = renderer->CreateShaderProgram();
  program->AddDefines(shader_defines);
  bool success = program->CompileAndLink(SKYBOX_VS, SKYBOX_FS);
  if (!success) {
    LOGE("create skybox program failed");
    return false;
  }
  model_skybox.material.shader_program = program;

  // uniforms
  auto uniforms_block_mvp = renderer->CreateUniformBlock("UniformsMVP", sizeof(UniformsMVP));
  model_skybox.material.uniform_groups = {{uniforms_block_mvp}, sampler_uniforms};

  UniformsMVP uniforms_mvp{};
  glm::mat4 model_matrix(1.f);
  ClearState clear_state;

  // fbo
  std::shared_ptr<FrameBuffer> fbo = renderer->CreateFrameBuffer();

  // draw
  fbo->Bind();
  renderer->SetViewPort(0, 0, tex_out->width, tex_out->height);

  for (int i = 0; i < 6; i++) {
    auto &param = captureViews[i];
    camera.LookAt(param.eye, param.center, param.up);

    // update mvp
    glm::mat4 view_matrix = glm::mat3(camera.ViewMatrix());  // only rotation
    uniforms_mvp.u_modelViewProjectionMatrix = camera.ProjectionMatrix() * view_matrix * model_matrix;
    uniforms_block_mvp->SetData(&uniforms_mvp, sizeof(UniformsMVP));

    // set color output
    fbo->SetColorAttachment(tex_out, CubeMapFace(TEXTURE_CUBE_MAP_POSITIVE_X + i));

    // draw
    renderer->Clear(clear_state);
    model_skybox.material.shader_program->Use();
    model_skybox.material.BindUniforms();
    renderer->SetVertexArray(model_skybox);
    renderer->SetRenderState(model_skybox.material.render_state);
    renderer->Draw(model_skybox.primitive_type);
  }

  return true;
}

bool Environment::GenerateIrradianceMap(std::shared_ptr<TextureCube> &tex_in,
                                        std::shared_ptr<TextureCube> &tex_out) {
  return false;
}

bool Environment::GeneratePrefilterMap(std::shared_ptr<TextureCube> &tex_in,
                                       std::shared_ptr<TextureCube> &tex_out) {
  return false;
}

}
}
