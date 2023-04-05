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
  if (!renderer) {
    LOGE("convertEquirectangular error: renderer nullptr");
    return false;
  }

  CubeRenderContext ctx;
  ctx.renderer = renderer;
  bool success = createCubeRenderContext(ctx,
                                         shaderFunc,
                                         std::dynamic_pointer_cast<Texture>(texIn),
                                         MaterialTexType_EQUIRECTANGULAR);
  if (!success) {
    LOGE("create render context failed");
    return false;
  }

  drawCubeFaces(ctx, texOut->width, texOut->height, texOut);
  return true;
}

bool Environment::generateIrradianceMap(const std::shared_ptr<Renderer> &renderer,
                                        const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                        const std::shared_ptr<Texture> &texIn,
                                        std::shared_ptr<Texture> &texOut) {
  if (!renderer) {
    LOGE("generateIrradianceMap error: renderer nullptr");
    return false;
  }

  CubeRenderContext ctx;
  ctx.renderer = renderer;
  bool success = createCubeRenderContext(ctx,
                                         shaderFunc,
                                         std::dynamic_pointer_cast<Texture>(texIn),
                                         MaterialTexType_CUBE);
  if (!success) {
    LOGE("create render context failed");
    return false;
  }

  drawCubeFaces(ctx, texOut->width, texOut->height, texOut);
  return true;
}

bool Environment::generatePrefilterMap(const std::shared_ptr<Renderer> &renderer,
                                       const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                       const std::shared_ptr<Texture> &texIn,
                                       std::shared_ptr<Texture> &texOut) {
  if (!renderer) {
    LOGE("generatePrefilterMap error: renderer nullptr");
    return false;
  }

  CubeRenderContext ctx;
  ctx.renderer = renderer;
  bool success = createCubeRenderContext(ctx,
                                         shaderFunc,
                                         std::dynamic_pointer_cast<Texture>(texIn),
                                         MaterialTexType_CUBE);
  if (!success) {
    LOGE("create render context failed");
    return false;
  }

  auto uniformsBlockPrefilter = ctx.renderer->createUniformBlock("UniformsPrefilter", sizeof(UniformsIBLPrefilter));
  ctx.modelSkybox.material->materialObj->shaderResources->blocks[UniformBlock_IBLPrefilter] = uniformsBlockPrefilter;

  UniformsIBLPrefilter uniformsPrefilter{};

  for (int level = 0; level < kPrefilterMaxMipLevels; level++) {
    drawCubeFaces(ctx, texOut->getLevelWidth(level), texOut->getLevelHeight(level), texOut, level, [&]() -> void {
      uniformsPrefilter.u_srcResolution = (float) texIn->width;
      uniformsPrefilter.u_roughness = (float) level / (float) (kPrefilterMaxMipLevels - 1);
      uniformsBlockPrefilter->setData(&uniformsPrefilter, sizeof(UniformsIBLPrefilter));
    });
  }
  return true;
}

bool Environment::createCubeRenderContext(CubeRenderContext &ctx,
                                          const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                          const std::shared_ptr<Texture> &texIn,
                                          MaterialTexType texType) {
  // camera
  ctx.camera.setPerspective(glm::radians(90.f), 1.f, 0.1f, 10.f);

  // model
  const std::string texName = "Environment";
  ModelLoader::loadCubeMesh(ctx.modelSkybox);
  ctx.modelSkybox.material = std::make_shared<Material>();
  ctx.modelSkybox.material->shadingModel = Shading_Skybox;
  ctx.modelSkybox.material->materialObj = std::make_shared<MaterialObject>();

  // fbo
  ctx.fbo = ctx.renderer->createFrameBuffer(true);

  // vao
  ctx.modelSkybox.vao = ctx.renderer->createVertexArrayObject(ctx.modelSkybox);

  // shader program
  std::set<std::string> shaderDefines = {Material::samplerDefine(texType)};
  auto program = ctx.renderer->createShaderProgram();
  program->addDefines(shaderDefines);
  bool success = shaderFunc(*program);
  if (!success) {
    LOGE("create shader program failed");
    return false;
  }
  ctx.modelSkybox.material->materialObj->shaderProgram = program;
  ctx.modelSkybox.material->materialObj->shaderResources = std::make_shared<ShaderResources>();

  // uniforms
  const char *samplerName = Material::samplerName(texType);
  auto uniform = ctx.renderer->createUniformSampler(samplerName, *texIn);
  uniform->setTexture(texIn);
  ctx.modelSkybox.material->materialObj->shaderResources->samplers[texType] = uniform;

  ctx.uniformsBlockModel = ctx.renderer->createUniformBlock("UniformsModel", sizeof(UniformsModel));
  ctx.modelSkybox.material->materialObj->shaderResources->blocks[UniformBlock_Model] = ctx.uniformsBlockModel;

  // pipeline
  ctx.modelSkybox.material->materialObj->pipelineStates = ctx.renderer->createPipelineStates({});

  return true;
}

void Environment::drawCubeFaces(CubeRenderContext &ctx, uint32_t width, uint32_t height, std::shared_ptr<Texture> &texOut,
                                uint32_t texOutLevel, const std::function<void()> &beforeDraw) {
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

  for (int i = 0; i < 6; i++) {
    auto &param = captureViews[i];
    ctx.camera.lookAt(param.eye, param.center, param.up);

    // update mvp
    glm::mat4 viewMatrix = glm::mat3(ctx.camera.viewMatrix());  // only rotation
    uniformsModel.u_modelViewProjectionMatrix = ctx.camera.projectionMatrix() * viewMatrix * modelMatrix;
    ctx.uniformsBlockModel->setData(&uniformsModel, sizeof(UniformsModel));

    if (beforeDraw) {
      beforeDraw();
    }

    auto &materialObj = ctx.modelSkybox.material->materialObj;
    ctx.fbo->setColorAttachment(texOut, CubeMapFace(TEXTURE_CUBE_MAP_POSITIVE_X + i), texOutLevel);

    ClearStates clearStates{};
    clearStates.colorFlag = true;

    // draw
    ctx.renderer->beginRenderPass(ctx.fbo, clearStates);
    ctx.renderer->setViewPort(0, 0, width, height);
    ctx.renderer->setVertexArrayObject(ctx.modelSkybox.vao);
    ctx.renderer->setShaderProgram(materialObj->shaderProgram);
    ctx.renderer->setShaderResources(materialObj->shaderResources);
    ctx.renderer->setPipelineStates(materialObj->pipelineStates);
    ctx.renderer->draw();
    ctx.renderer->endRenderPass();
  }
}

}
}
