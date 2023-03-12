/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "Environment.h"
#include "ModelLoader.h"
#include "Base/Logger.h"

namespace SoftGL {
namespace View {

struct LookAtParam {
  glm::vec3 eye;
  glm::vec3 center;
  glm::vec3 up;
};

bool Environment::convertEquirectangular(const std::shared_ptr<Renderer> &renderer,
                                         const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                         const std::shared_ptr<Texture> &texIn,
                                         std::shared_ptr<Texture> &texOut) {
  CubeRenderContext context;
  context.renderer = renderer;
  bool success = createCubeRenderContext(context,
                                         shaderFunc,
                                         std::dynamic_pointer_cast<Texture>(texIn),
                                         TextureUsage_EQUIRECTANGULAR);
  if (!success) {
    LOGE("create render context failed");
    return false;
  }

  drawCubeFaces(context, texOut->width, texOut->height, texOut);
  return true;
}

bool Environment::generateIrradianceMap(const std::shared_ptr<Renderer> &renderer,
                                        const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                        const std::shared_ptr<Texture> &texIn,
                                        std::shared_ptr<Texture> &texOut) {
  CubeRenderContext context;
  context.renderer = renderer;
  bool success = createCubeRenderContext(context,
                                         shaderFunc,
                                         std::dynamic_pointer_cast<Texture>(texIn),
                                         TextureUsage_CUBE);
  if (!success) {
    LOGE("create render context failed");
    return false;
  }

  drawCubeFaces(context, texOut->width, texOut->height, texOut);
  return true;
}

bool Environment::generatePrefilterMap(const std::shared_ptr<Renderer> &renderer,
                                       const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                       const std::shared_ptr<Texture> &texIn,
                                       std::shared_ptr<Texture> &texOut) {
  CubeRenderContext context;
  context.renderer = renderer;
  bool success = createCubeRenderContext(context,
                                         shaderFunc,
                                         std::dynamic_pointer_cast<Texture>(texIn),
                                         TextureUsage_CUBE);
  if (!success) {
    LOGE("create render context failed");
    return false;
  }

  auto uniformsBlockPrefilter =
      context.renderer->createUniformBlock("UniformsPrefilter", sizeof(UniformsIBLPrefilter));
  context.modelSkybox.material->shaderUniforms->blocks[UniformBlock_IBLPrefilter] = uniformsBlockPrefilter;

  UniformsIBLPrefilter uniformsPrefilter{};

  for (int mip = 0; mip < kPrefilterMaxMipLevels; mip++) {
    int mipWidth = (int) (texOut->width * glm::pow(0.5f, mip));
    int mipHeight = (int) (texOut->height * glm::pow(0.5f, mip));

    drawCubeFaces(context, mipWidth, mipHeight, texOut, mip, [&]() -> void {
      uniformsPrefilter.u_srcResolution = (float) texIn->width;
      uniformsPrefilter.u_roughness = (float) mip / (float) (kPrefilterMaxMipLevels - 1);
      uniformsBlockPrefilter->setData(&uniformsPrefilter, sizeof(UniformsIBLPrefilter));
    });
  }
  return true;
}

bool Environment::createCubeRenderContext(CubeRenderContext &context,
                                          const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                          const std::shared_ptr<Texture> &texIn,
                                          TextureUsage texUsage) {
  // camera
  context.camera.setPerspective(glm::radians(90.f), 1.f, 0.1f, 10.f);

  // model
  const std::string texName = "Environment";
  ModelLoader::loadCubeMesh(context.modelSkybox);
  context.modelSkybox.materialCache[texName] = {};
  context.modelSkybox.material = &context.modelSkybox.materialCache[texName];
  context.modelSkybox.material->reset();
  context.modelSkybox.material->shading = Shading_Skybox;

  // fbo
  context.fbo = context.renderer->createFrameBuffer();

  // vao
  context.modelSkybox.vao = context.renderer->createVertexArrayObject(context.modelSkybox);

  // shader program
  std::set<std::string> shaderDefines = {Material::samplerDefine(texUsage)};
  auto program = context.renderer->createShaderProgram();
  program->addDefines(shaderDefines);
  bool success = shaderFunc(*program);
  if (!success) {
    LOGE("create shader program failed");
    return false;
  }
  context.modelSkybox.material->shaderProgram = program;
  context.modelSkybox.material->shaderUniforms = std::make_shared<ShaderUniforms>();

  // uniforms
  const char *samplerName = Material::samplerName(texUsage);
  auto uniform = context.renderer->createUniformSampler(samplerName, texIn->type, texIn->format);
  uniform->setTexture(texIn);
  context.modelSkybox.material->shaderUniforms->samplers[texUsage] = uniform;

  context.uniformsBlockModel = context.renderer->createUniformBlock("UniformsModel", sizeof(UniformsModel));
  context.modelSkybox.material->shaderUniforms->blocks[UniformBlock_Model] = context.uniformsBlockModel;

  return true;
}

void Environment::drawCubeFaces(CubeRenderContext &context, int width, int height, std::shared_ptr<Texture> &texOut,
                                int texOutLevel, const std::function<void()> &beforeDraw) {
  static LookAtParam captureViews[] = {
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)}
  };

  UniformsModel uniformsModel{};
  glm::mat4 modelMatrix(1.f);

  // draw
  context.renderer->setFrameBuffer(context.fbo);
  context.renderer->setViewPort(0, 0, width, height);

  for (int i = 0; i < 6; i++) {
    auto &param = captureViews[i];
    context.camera.lookAt(param.eye, param.center, param.up);

    // update mvp
    glm::mat4 viewMatrix = glm::mat3(context.camera.viewMatrix());  // only rotation
    uniformsModel.u_modelViewProjectionMatrix = context.camera.projectionMatrix() * viewMatrix * modelMatrix;
    context.uniformsBlockModel->setData(&uniformsModel, sizeof(UniformsModel));

    if (beforeDraw) {
      beforeDraw();
    }

    // draw
    context.fbo->setColorAttachment(texOut, CubeMapFace(TEXTURE_CUBE_MAP_POSITIVE_X + i), texOutLevel);
    context.renderer->clear({});
    context.renderer->setVertexArrayObject(context.modelSkybox.vao);
    context.renderer->setRenderState(context.modelSkybox.material->renderState);
    context.renderer->setShaderProgram(context.modelSkybox.material->shaderProgram);
    context.renderer->setShaderUniforms(context.modelSkybox.material->shaderUniforms);
    context.renderer->draw(context.modelSkybox.primitiveType);
  }
}

}
}
