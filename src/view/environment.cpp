/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "environment.h"
#include "model_loader.h"
#include "camera.h"
#include "render/soft/shader/skybox_shader.h"
#include "render/soft/shader/irradiance_shader.h"
#include "render/soft/shader/prefilter_shader.h"

namespace SoftGL {
namespace View {

#define CREATE_SHADER(PREFIX)                                                   \
  shader_context.vertex_shader = std::make_shared<PREFIX##VertexShader>();      \
  shader_context.fragment_shader = std::make_shared<PREFIX##FragmentShader>();  \
  shader_context.uniforms = std::make_shared<PREFIX##ShaderUniforms>();         \
  shader_context.varyings_size = sizeof(PREFIX##ShaderVaryings);                \
  shader_context.vertex_shader->uniforms = shader_context.uniforms.get();       \
  shader_context.fragment_shader->uniforms = shader_context.uniforms.get();     \


#define IrradianceMapSize 32
#define PrefilterMaxMipLevels 5
#define PrefilterMapSize 128

struct LookAtParam {
  glm::vec3 eye;
  glm::vec3 center;
  glm::vec3 up;
};

void Environment::ConvertEquirectangular(Texture &tex_in, Texture *tex_out, int width, int height) {
  int default_size = std::min(tex_in.buffer->GetWidth(), tex_in.buffer->GetHeight());
  if (width <= 0) width = default_size;
  if (height <= 0) height = default_size;

  Camera camera;
  auto renderer = CreateCubeRender(camera, width, height);

  // init shader
  auto &shader_context = renderer->GetShaderContext();
  CREATE_SHADER(Skybox);

  auto *skybox_uniforms_ptr = (SkyboxShaderUniforms *) shader_context.uniforms.get();
  skybox_uniforms_ptr->u_equirectangularMap.SetTexture(&tex_in);
  skybox_uniforms_ptr->u_equirectangularMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
  skybox_uniforms_ptr->u_equirectangularMap.SetFilterMode(Filter_LINEAR);

  // draw
  CubeRenderDraw(*renderer, camera, [&](int face, Buffer<glm::u8vec4> &buff) {
    tex_out[face].buffer = Texture::CreateBuffer(buff.GetLayout());
    tex_out[face].buffer->Create(buff.GetWidth(), buff.GetHeight());
    buff.CopyRawDataTo(tex_out[face].buffer->GetRawDataPtr(), true);
  });
}

void Environment::GenerateIrradianceMap(Texture *tex_in, Texture *tex_out) {
  Camera camera;
  auto renderer = CreateCubeRender(camera, IrradianceMapSize, IrradianceMapSize);

  // init shader
  auto &shader_context = renderer->GetShaderContext();
  CREATE_SHADER(Irradiance);

  auto *skybox_uniforms_ptr = (IrradianceShaderUniforms *) shader_context.uniforms.get();
  for (int i = 0; i < 6; i++) {
    skybox_uniforms_ptr->u_cubeMap.SetTexture(&tex_in[i], i);
  }
  skybox_uniforms_ptr->u_cubeMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
  skybox_uniforms_ptr->u_cubeMap.SetFilterMode(Filter_LINEAR);

  // draw
  CubeRenderDraw(*renderer, camera, [&](int face, Buffer<glm::u8vec4> &buff) {
    tex_out[face].buffer = Texture::CreateBuffer(buff.GetLayout());
    tex_out[face].buffer->Create(buff.GetWidth(), buff.GetHeight());
    buff.CopyRawDataTo(tex_out[face].buffer->GetRawDataPtr(), true);
  });
}

void Environment::GeneratePrefilterMap(Texture *tex_in, Texture *tex_out) {
  for (int i = 0; i < 6; i++) {
    tex_out[i].buffer = Texture::CreateBuffer(tex_in->buffer->GetLayout());
    tex_out[i].buffer->Create(tex_in->buffer->GetWidth(), tex_in->buffer->GetHeight());
    tex_in->buffer->CopyRawDataTo(tex_out[i].buffer->GetRawDataPtr());

    tex_out[i].mipmaps.resize(PrefilterMaxMipLevels);
    tex_out[i].mipmaps_ready = true;
  }

  for (int mip = 0; mip < PrefilterMaxMipLevels; mip++) {
    int mip_width = PrefilterMapSize * glm::pow(0.5f, mip);
    int mip_height = PrefilterMapSize * glm::pow(0.5f, mip);
    GeneratePrefilterMapLod(tex_in, tex_out, mip_width, mip_height, mip);
  }
}

void Environment::GeneratePrefilterMapLod(Texture *tex_in,
                                          Texture *tex_out,
                                          int width,
                                          int height,
                                          int mip) {
  Camera camera;
  auto renderer = CreateCubeRender(camera, width, height);

  // init shader
  auto &shader_context = renderer->GetShaderContext();
  CREATE_SHADER(Prefilter);

  auto *skybox_uniforms_ptr = (PrefilterShaderUniforms *) shader_context.uniforms.get();
  skybox_uniforms_ptr->u_srcResolution = tex_in[0].buffer->GetWidth();
  skybox_uniforms_ptr->u_roughness = (float) mip / (float) (PrefilterMaxMipLevels - 1);
  for (int i = 0; i < 6; i++) {
    skybox_uniforms_ptr->u_cubeMap.SetTexture(&tex_in[i], i);
  }
  skybox_uniforms_ptr->u_cubeMap.SetWrapMode(Wrap_CLAMP_TO_EDGE);
  skybox_uniforms_ptr->u_cubeMap.SetFilterMode(Filter_LINEAR);

  // draw
  CubeRenderDraw(*renderer, camera, [&](int face, Buffer<glm::u8vec4> &buff) {
    tex_out[face].mipmaps[mip] = Texture::CreateBuffer(buff.GetLayout());
    auto &dst_buff = tex_out[face].mipmaps[mip];

    dst_buff->Create(buff.GetWidth(), buff.GetHeight());
    buff.CopyRawDataTo(dst_buff->GetRawDataPtr(), true);
  });
}

std::shared_ptr<RendererSoft> Environment::CreateCubeRender(Camera &camera, int width, int height) {
  float cam_near = 0.1f;
  float cam_far = 10.f;
  camera.SetPerspective(glm::radians(90.f), 1.f, cam_near, cam_far);
  auto renderer = std::make_shared<RendererSoft>();
  renderer->Create(width, height, cam_near, cam_far);
  renderer->early_z = false;
  renderer->depth_test = false;
  renderer->cull_face_back = false;
  return renderer;
}

void Environment::CubeRenderDraw(RendererSoft &renderer,
                                 Camera &camera,
                                 const std::function<void(int face, Buffer<glm::u8vec4> &buff)> &face_cb) {
  LookAtParam captureViews[] = {
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)}
  };

  auto &shader_context = renderer.GetShaderContext();
  auto *uniforms_ptr = (BaseShaderUniforms *) shader_context.uniforms.get();
  auto skybox_mesh = ModelLoader::GetSkyBoxMesh();
  glm::mat4 model_matrix(1.f);

  // draw cube 6 faces
  for (int i = 0; i < 6; i++) {
    auto &param = captureViews[i];
    camera.LookAt(param.eye, param.center, param.up);
    uniforms_ptr->u_modelViewProjectionMatrix = camera.ProjectionMatrix() * camera.ViewMatrix() * model_matrix;
    renderer.Clear(0.f, 0.f, 0.f, 0.f);
    renderer.DrawMeshTextured(*skybox_mesh);

    auto &buff = renderer.GetFrameColor();
    face_cb(i, *buff);
  }
}

}
}