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

// TODO enable cache

bool IBLGenerator::convertEquirectangular(const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                          const std::shared_ptr<Texture> &texIn,
                                          std::shared_ptr<Texture> &texOut) {
  contextCache_.push_back(std::make_shared<CubeRenderContext>());
  CubeRenderContext &ctx = *contextCache_.back();
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

bool IBLGenerator::generateIrradianceMap(const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                         const std::shared_ptr<Texture> &texIn,
                                         std::shared_ptr<Texture> &texOut) {
  contextCache_.push_back(std::make_shared<CubeRenderContext>());
  CubeRenderContext &ctx = *contextCache_.back();
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

bool IBLGenerator::generatePrefilterMap(const std::function<bool(ShaderProgram &program)> &shaderFunc,
                                        const std::shared_ptr<Texture> &texIn,
                                        std::shared_ptr<Texture> &texOut) {
  contextCache_.push_back(std::make_shared<CubeRenderContext>());
  CubeRenderContext &ctx = *contextCache_.back();
  bool success = createCubeRenderContext(ctx,
                                         shaderFunc,
                                         std::dynamic_pointer_cast<Texture>(texIn),
                                         MaterialTexType_CUBE);
  if (!success) {
    LOGE("create render context failed");
    return false;
  }

  auto uniformsBlockPrefilter = renderer_->createUniformBlock("UniformsPrefilter", sizeof(UniformsIBLPrefilter));
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

bool IBLGenerator::createCubeRenderContext(CubeRenderContext &ctx,
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
  ctx.fbo = renderer_->createFrameBuffer(true);

  // vao
  ctx.modelSkybox.vao = renderer_->createVertexArrayObject(ctx.modelSkybox);

  // shader program
  std::set<std::string> shaderDefines = {Material::samplerDefine(texType)};
  auto program = renderer_->createShaderProgram();
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
  auto uniform = renderer_->createUniformSampler(samplerName, *texIn);
  uniform->setTexture(texIn);
  ctx.modelSkybox.material->materialObj->shaderResources->samplers[texType] = uniform;

  ctx.uniformsBlockModel = renderer_->createUniformBlock("UniformsModel", sizeof(UniformsModel));
  ctx.modelSkybox.material->materialObj->shaderResources->blocks[UniformBlock_Model] = ctx.uniformsBlockModel;

  // pipeline
  ctx.modelSkybox.material->materialObj->pipelineStates = renderer_->createPipelineStates({});

  return true;
}

void IBLGenerator::drawCubeFaces(CubeRenderContext &ctx, uint32_t width, uint32_t height, std::shared_ptr<Texture> &texOut,
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
    renderer_->beginRenderPass(ctx.fbo, clearStates);
    renderer_->setViewPort(0, 0, width, height);
    renderer_->setVertexArrayObject(ctx.modelSkybox.vao);
    renderer_->setShaderProgram(materialObj->shaderProgram);
    renderer_->setShaderResources(materialObj->shaderResources);
    renderer_->setPipelineStates(materialObj->pipelineStates);
    renderer_->draw();
    renderer_->endRenderPass();
  }
}

}
}
