/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "Viewer.h"
#include <algorithm>
#include "Base/Logger.h"
#include "Base/HashUtils.h"
#include "Environment.h"

namespace SoftGL {
namespace View {

#define SHADOW_MAP_WIDTH 512
#define SHADOW_MAP_HEIGHT 512

#define CREATE_UNIFORM_BLOCK(name) renderer_->createUniformBlock(#name, sizeof(name))

bool Viewer::create(int width, int height, int outTexId) {
  cleanup();

  width_ = width;
  height_ = height;
  outTexId_ = outTexId;

  // main camera
  camera_ = &cameraMain_;

  // depth camera
  if (!cameraDepth_) {
    cameraDepth_ = std::make_shared<Camera>();
    cameraDepth_->setPerspective(glm::radians(CAMERA_FOV),
                                 (float) SHADOW_MAP_WIDTH / (float) SHADOW_MAP_HEIGHT,
                                 CAMERA_NEAR, CAMERA_FAR);

  }

  // create renderer
  if (!renderer_) {
    renderer_ = createRenderer();
  }
  if (!renderer_) {
    LOGE("Viewer::create failed: createRenderer error");
    return false;
  }

  // create default resources
  uniformBlockScene_ = CREATE_UNIFORM_BLOCK(UniformsScene);
  uniformBlockModel_ = CREATE_UNIFORM_BLOCK(UniformsModel);
  uniformBlockMaterial_ = CREATE_UNIFORM_BLOCK(UniformsMaterial);

  shadowPlaceholder_ = createTexture2DDefault(1, 1, TextureFormat_FLOAT32, TextureUsage_Sampler);
  iblPlaceholder_ = createTextureCubeDefault(1, 1, TextureUsage_Sampler);

  return true;
}

void Viewer::destroy() {
  cleanup();
  cameraDepth_ = nullptr;
  if (renderer_) {
    renderer_->destroy();
  }
  renderer_ = nullptr;
}

void Viewer::cleanup() {
  fboMain_ = nullptr;
  texColorMain_ = nullptr;
  texDepthMain_ = nullptr;
  fboShadow_ = nullptr;
  texDepthShadow_ = nullptr;
  shadowPlaceholder_ = nullptr;
  fxaaFilter_ = nullptr;
  texColorFxaa_ = nullptr;
  iblPlaceholder_ = nullptr;
  uniformBlockScene_ = nullptr;
  uniformBlockModel_ = nullptr;
  uniformBlockMaterial_ = nullptr;
  programCache_.clear();
  pipelineCache_.clear();
}

void Viewer::onResetReverseZ() {
  texDepthShadow_ = nullptr;
}

void Viewer::drawFrame(DemoScene &scene) {
  if (!renderer_) {
    return;
  }

  scene_ = &scene;

  // setup framebuffer
  setupMainBuffers();
  setupShadowMapBuffers();

  // init skybox textures & ibl
  initSkyboxIBL();

  // setup model materials
  setupScene();

  // draw shadow map
  drawShadowMap();

  // setup fxaa
  processFXAASetup();

  // main pass
  ClearStates clearStates{};
  clearStates.colorFlag = true;
  clearStates.depthFlag = config_.depthTest;
  clearStates.clearColor = config_.clearColor;
  clearStates.clearDepth = config_.reverseZ ? 0.f : 1.f;

  renderer_->beginRenderPass(fboMain_, clearStates);
  renderer_->setViewPort(0, 0, width_, height_);

  // draw scene
  drawScene(false);

  // end main pass
  renderer_->endRenderPass();

  // draw fxaa
  processFXAADraw();
}

void Viewer::drawShadowMap() {
  if (!config_.shadowMap) {
    return;
  }

  // shadow pass
  ClearStates clearDepth{};
  clearDepth.depthFlag = true;
  clearDepth.clearDepth = config_.reverseZ ? 0.f : 1.f;
  renderer_->beginRenderPass(fboShadow_, clearDepth);
  renderer_->setViewPort(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

  // set camera
  cameraDepth_->lookAt(config_.pointLightPosition, glm::vec3(0), glm::vec3(0, 1, 0));
  cameraDepth_->update();
  camera_ = cameraDepth_.get();

  // draw scene
  drawScene(true);

  // end shadow pass
  renderer_->endRenderPass();

  // set back to main camera
  camera_ = &cameraMain_;
}

void Viewer::processFXAASetup() {
  if (config_.aaType != AAType_FXAA) {
    return;
  }

  if (!texColorFxaa_) {
    TextureDesc texDesc{};
    texDesc.width = width_;
    texDesc.height = height_;
    texDesc.type = TextureType_2D;
    texDesc.format = TextureFormat_RGBA8;
    texDesc.usage = TextureUsage_Sampler | TextureUsage_AttachmentColor;
    texDesc.useMipmaps = false;
    texDesc.multiSample = false;
    texColorFxaa_ = renderer_->createTexture(texDesc);

    SamplerDesc sampler{};
    sampler.filterMin = Filter_LINEAR;
    sampler.filterMag = Filter_LINEAR;
    texColorFxaa_->setSamplerDesc(sampler);

    texColorFxaa_->initImageData();
  }

  if (!fxaaFilter_) {
    fxaaFilter_ = std::make_shared<QuadFilter>(width_, height_, renderer_,
                                               [&](ShaderProgram &program) -> bool {
                                                 return loadShaders(program, Shading_FXAA);
                                               });
  }

  fboMain_->setColorAttachment(texColorFxaa_, 0);
  fboMain_->setOffscreen(true);

  fxaaFilter_->setTextures(texColorFxaa_, texColorMain_);
}

void Viewer::processFXAADraw() {
  if (config_.aaType != AAType_FXAA) {
    return;
  }

  fxaaFilter_->draw();
}

void Viewer::setupScene() {
  // point light
  if (config_.showLight) {
    setupPoints(scene_->pointLight);
  }

  // world axis
  if (config_.worldAxis) {
    setupLines(scene_->worldAxis);
  }

  // floor
  if (config_.showFloor) {
    if (config_.wireframe) {
      setupMeshBaseColor(scene_->floor, true);
    } else {
      setupMeshTextured(scene_->floor);
    }
  }

  // skybox
  if (config_.showSkybox) {
    setupSkybox(scene_->skybox);
  }

  // model nodes
  ModelNode &modelNode = scene_->model->rootNode;
  setupModelNodes(modelNode, config_.wireframe);
}

void Viewer::setupPoints(ModelPoints &points) {
  pipelineSetup(points, points.material->shadingModel,
                {UniformBlock_Model, UniformBlock_Material});
}

void Viewer::setupLines(ModelLines &lines) {
  pipelineSetup(lines, lines.material->shadingModel,
                {UniformBlock_Model, UniformBlock_Material});
}

void Viewer::setupSkybox(ModelMesh &skybox) {
  pipelineSetup(skybox, skybox.material->shadingModel,
                {UniformBlock_Model},
                [&](RenderStates &rs) -> void {
                  rs.depthFunc = config_.reverseZ ? DepthFunc_GEQUAL : DepthFunc_LEQUAL;
                  rs.depthMask = false;
                });
}

void Viewer::setupMeshBaseColor(ModelMesh &mesh, bool wireframe) {
  pipelineSetup(mesh, Shading_BaseColor,
                {UniformBlock_Model, UniformBlock_Scene, UniformBlock_Material},
                [&](RenderStates &rs) -> void {
                  rs.polygonMode = wireframe ? PolygonMode_LINE : PolygonMode_FILL;
                });
}

void Viewer::setupMeshTextured(ModelMesh &mesh) {
  pipelineSetup(mesh, mesh.material->shadingModel,
                {UniformBlock_Model, UniformBlock_Scene, UniformBlock_Material});
}

void Viewer::setupModelNodes(ModelNode &node, bool wireframe) {
  for (auto &mesh : node.meshes) {
    // setup mesh
    if (wireframe) {
      setupMeshBaseColor(mesh, true);
    } else {
      setupMeshTextured(mesh);
    }
  }

  // setup child
  for (auto &childNode : node.children) {
    setupModelNodes(childNode, wireframe);
  }
}

void Viewer::drawScene(bool shadowPass) {
  // update scene uniform
  updateUniformScene();
  updateUniformModel(glm::mat4(1.0f), camera_->viewMatrix());

  // draw point light
  if (!shadowPass && config_.showLight) {
    updateUniformMaterial(*scene_->pointLight.material);
    pipelineDraw(scene_->pointLight);
  }

  // draw world axis
  if (!shadowPass && config_.worldAxis) {
    updateUniformMaterial(*scene_->worldAxis.material);
    pipelineDraw(scene_->worldAxis);
  }

  // draw floor
  if (!shadowPass && config_.showFloor) {
    drawModelMesh(scene_->floor, 0.f);
  }

  // draw model nodes opaque
  ModelNode &modelNode = scene_->model->rootNode;
  drawModelNodes(modelNode, scene_->model->centeredTransform, Alpha_Opaque);

  // draw skybox
  if (!shadowPass && config_.showSkybox) {
    // view matrix only rotation
    updateUniformModel(glm::mat4(1.0f), glm::mat3(camera_->viewMatrix()));
    pipelineDraw(scene_->skybox);
  }

  // draw model nodes blend
  drawModelNodes(modelNode, scene_->model->centeredTransform, Alpha_Blend);
}

void Viewer::drawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, float specular) {
  glm::mat4 modelMatrix = transform * node.transform;

  // update model uniform
  updateUniformModel(modelMatrix, camera_->viewMatrix());

  // draw nodes
  for (auto &mesh : node.meshes) {
    if (mesh.material->alphaMode != mode) {
      continue;
    }

    // frustum cull
    if (!checkMeshFrustumCull(mesh, modelMatrix)) {
      return;
    }

    drawModelMesh(mesh, specular);
  }

  // draw child
  for (auto &childNode : node.children) {
    drawModelNodes(childNode, modelMatrix, mode, specular);
  }
}

void Viewer::drawModelMesh(ModelMesh &mesh, float specular) {
  // update material
  updateUniformMaterial(*mesh.material, specular);

  // update IBL textures
  if (mesh.material->shadingModel == Shading_PBR) {
    updateIBLTextures(mesh.material->materialObj.get());
  }

  // update shadow textures
  if (config_.shadowMap) {
    updateShadowTextures(mesh.material->materialObj.get());
  }

  // draw mesh
  pipelineDraw(mesh);
}

void Viewer::pipelineSetup(ModelBase &model, ShadingModel shading, const std::set<int> &uniformBlocks,
                           const std::function<void(RenderStates &rs)> &extraStates) {
  setupVertexArray(model);

  // reset materialObj if ShadingModel changed
  if (model.material->materialObj != nullptr) {
    if (model.material->materialObj->shadingModel != shading) {
      model.material->materialObj = nullptr;
    }
  }

  setupMaterial(model, shading, uniformBlocks, extraStates);
}

void Viewer::pipelineDraw(ModelBase &model) {
  auto &materialObj = model.material->materialObj;

  renderer_->setVertexArrayObject(model.vao);
  renderer_->setShaderProgram(materialObj->shaderProgram);
  renderer_->setShaderResources(materialObj->shaderResources);
  renderer_->setPipelineStates(materialObj->pipelineStates);
  renderer_->draw();
}

void Viewer::setupMainBuffers() {
  if (config_.aaType == AAType_MSAA) {
    setupMainColorBuffer(true);
    setupMainDepthBuffer(true);
  } else {
    setupMainColorBuffer(false);
    setupMainDepthBuffer(false);
  }

  if (!fboMain_) {
    fboMain_ = renderer_->createFrameBuffer(false);
  }
  fboMain_->setColorAttachment(texColorMain_, 0);
  fboMain_->setDepthAttachment(texDepthMain_);
  fboMain_->setOffscreen(false);

  if (!fboMain_->isValid()) {
    LOGE("setupMainBuffers failed");
  }
}

void Viewer::setupShadowMapBuffers() {
  if (!config_.shadowMap) {
    return;
  }

  if (!fboShadow_) {
    fboShadow_ = renderer_->createFrameBuffer(true);
  }

  if (!texDepthShadow_) {
    TextureDesc texDesc{};
    texDesc.width = SHADOW_MAP_WIDTH;
    texDesc.height = SHADOW_MAP_HEIGHT;
    texDesc.type = TextureType_2D;
    texDesc.format = TextureFormat_FLOAT32;
    texDesc.usage = TextureUsage_Sampler | TextureUsage_AttachmentDepth;
    texDesc.useMipmaps = false;
    texDesc.multiSample = false;
    texDepthShadow_ = renderer_->createTexture(texDesc);

    SamplerDesc sampler{};
    sampler.filterMin = Filter_NEAREST;
    sampler.filterMag = Filter_NEAREST;
    sampler.wrapS = Wrap_CLAMP_TO_BORDER;
    sampler.wrapT = Wrap_CLAMP_TO_BORDER;
    sampler.borderColor = config_.reverseZ ? Border_BLACK : Border_WHITE;
    texDepthShadow_->setSamplerDesc(sampler);

    texDepthShadow_->initImageData();
    fboShadow_->setDepthAttachment(texDepthShadow_);

    if (!fboShadow_->isValid()) {
      LOGE("setupShadowMapBuffers failed");
    }
  }
}

void Viewer::setupMainColorBuffer(bool multiSample) {
  if (!texColorMain_ || texColorMain_->multiSample != multiSample) {
    TextureDesc texDesc{};
    texDesc.width = width_;
    texDesc.height = height_;
    texDesc.type = TextureType_2D;
    texDesc.format = TextureFormat_RGBA8;
    texDesc.usage = TextureUsage_AttachmentColor;
    texDesc.useMipmaps = false;
    texDesc.multiSample = multiSample;
    texColorMain_ = renderer_->createTexture(texDesc);

    SamplerDesc sampler{};
    sampler.filterMin = Filter_LINEAR;
    sampler.filterMag = Filter_LINEAR;
    texColorMain_->setSamplerDesc(sampler);

    texColorMain_->initImageData();
  }
}

void Viewer::setupMainDepthBuffer(bool multiSample) {
  if (!texDepthMain_ || texDepthMain_->multiSample != multiSample) {
    TextureDesc texDesc{};
    texDesc.width = width_;
    texDesc.height = height_;
    texDesc.type = TextureType_2D;
    texDesc.format = TextureFormat_FLOAT32;
    texDesc.usage = TextureUsage_AttachmentDepth;
    texDesc.useMipmaps = false;
    texDesc.multiSample = multiSample;
    texDepthMain_ = renderer_->createTexture(texDesc);

    SamplerDesc sampler{};
    sampler.filterMin = Filter_NEAREST;
    sampler.filterMag = Filter_NEAREST;
    texDepthMain_->setSamplerDesc(sampler);

    texDepthMain_->initImageData();
  }
}

void Viewer::setupVertexArray(ModelVertexes &vertexes) {
  if (!vertexes.vao) {
    vertexes.vao = renderer_->createVertexArrayObject(vertexes);
  }
}

void Viewer::setupTextures(Material &material) {
  for (auto &kv : material.textureData) {
    TextureDesc texDesc{};
    texDesc.width = (int) kv.second.width;
    texDesc.height = (int) kv.second.height;
    texDesc.format = TextureFormat_RGBA8;
    texDesc.usage = TextureUsage_Sampler | TextureUsage_UploadData;
    texDesc.useMipmaps = false;
    texDesc.multiSample = false;

    SamplerDesc sampler{};
    sampler.wrapS = kv.second.wrapMode;
    sampler.wrapT = kv.second.wrapMode;
    sampler.filterMin = Filter_LINEAR;
    sampler.filterMag = Filter_LINEAR;

    std::shared_ptr<Texture> texture = nullptr;
    switch (kv.first) {
      case MaterialTexType_IBL_IRRADIANCE:
      case MaterialTexType_IBL_PREFILTER: {
        // skip ibl textures
        break;
      }
      case MaterialTexType_CUBE: {
        texDesc.type = TextureType_CUBE;
        sampler.wrapR = kv.second.wrapMode;
        break;
      }
      default: {
        texDesc.type = TextureType_2D;
        texDesc.useMipmaps = config_.mipmaps;
        sampler.filterMin = config_.mipmaps ? Filter_LINEAR_MIPMAP_LINEAR : Filter_LINEAR;
        break;
      }
    }
    texture = renderer_->createTexture(texDesc);
    texture->setSamplerDesc(sampler);
    texture->setImageData(kv.second.data);
    material.textures[kv.first] = texture;
  }

  // default shadow depth texture
  if (material.shadingModel != Shading_Skybox) {
    material.textures[MaterialTexType_SHADOWMAP] = shadowPlaceholder_;
  }

  // default IBL texture
  if (material.shadingModel == Shading_PBR) {
    material.textures[MaterialTexType_IBL_IRRADIANCE] = iblPlaceholder_;
    material.textures[MaterialTexType_IBL_PREFILTER] = iblPlaceholder_;
  }
}

void Viewer::setupSamplerUniforms(Material &material) {
  for (auto &kv : material.textures) {
    // create sampler uniform
    const char *samplerName = Material::samplerName((MaterialTexType) kv.first);
    if (samplerName) {
      auto uniform = renderer_->createUniformSampler(samplerName, *kv.second);
      uniform->setTexture(kv.second);
      material.materialObj->shaderResources->samplers[kv.first] = std::move(uniform);
    }
  }
}

bool Viewer::setupShaderProgram(Material &material, ShadingModel shading) {
  size_t cacheKey = getShaderProgramCacheKey(shading, material.shaderDefines);

  // try cache
  auto cachedProgram = programCache_.find(cacheKey);
  if (cachedProgram != programCache_.end()) {
    material.materialObj->shaderProgram = cachedProgram->second;
    material.materialObj->shaderResources = std::make_shared<ShaderResources>();
    return true;
  }

  auto program = renderer_->createShaderProgram();
  program->addDefines(material.shaderDefines);

  bool success = loadShaders(*program, shading);
  if (success) {
    // add to cache
    programCache_[cacheKey] = program;
    material.materialObj->shaderProgram = program;
    material.materialObj->shaderResources = std::make_shared<ShaderResources>();
  } else {
    LOGE("setupShaderProgram failed: %s", Material::shadingModelStr(shading));
  }

  return success;
}

void Viewer::setupPipelineStates(ModelBase &model, const std::function<void(RenderStates &rs)> &extraStates) {
  auto &material = *model.material;
  RenderStates rs;
  rs.blend = material.alphaMode == Alpha_Blend;
  rs.blendParams.SetBlendFactor(BlendFactor_SRC_ALPHA, BlendFactor_ONE_MINUS_SRC_ALPHA);

  rs.depthTest = config_.depthTest;
  rs.depthMask = !rs.blend;   // disable depth write if blending enabled
  rs.depthFunc = config_.reverseZ ? DepthFunc_GREATER : DepthFunc_LESS;

  rs.cullFace = config_.cullFace && (!material.doubleSided);
  rs.primitiveType = model.primitiveType;
  rs.polygonMode = PolygonMode_FILL;

  rs.lineWidth = material.lineWidth;

  if (extraStates) {
    extraStates(rs);
  }

  size_t cacheKey = getPipelineCacheKey(material, rs);
  auto it = pipelineCache_.find(cacheKey);
  if (it != pipelineCache_.end()) {
    material.materialObj->pipelineStates = it->second;
  } else {
    auto pipelineStates = renderer_->createPipelineStates(rs);
    material.materialObj->pipelineStates = pipelineStates;
    pipelineCache_[cacheKey] = pipelineStates;
  }
}

void Viewer::setupMaterial(ModelBase &model, ShadingModel shading, const std::set<int> &uniformBlocks,
                           const std::function<void(RenderStates &rs)> &extraStates) {
  auto &material = *model.material;
  if (material.textures.empty()) {
    setupTextures(material);
    material.shaderDefines = generateShaderDefines(material);
  }

  if (!material.materialObj) {
    material.materialObj = std::make_shared<MaterialObject>();
    material.materialObj->shadingModel = shading;

    // setup uniform samplers
    if (setupShaderProgram(material, shading)) {
      setupSamplerUniforms(material);
    }

    // setup uniform blocks
    for (auto &key : uniformBlocks) {
      std::shared_ptr<UniformBlock> uniform = nullptr;
      switch (key) {
        case UniformBlock_Scene: {
          uniform = uniformBlockScene_;
          break;
        }
        case UniformBlock_Model: {
          uniform = uniformBlockModel_;
          break;
        }
        case UniformBlock_Material: {
          uniform = uniformBlockMaterial_;
          break;
        }
        default:
          break;
      }
      if (uniform) {
        material.materialObj->shaderResources->blocks[key] = uniform;
      }
    }
  }

  setupPipelineStates(model, extraStates);
}

void Viewer::updateUniformScene() {
  static UniformsScene uniformsScene{};

  uniformsScene.u_ambientColor = config_.ambientColor;
  uniformsScene.u_cameraPosition = camera_->eye();
  uniformsScene.u_pointLightPosition = config_.pointLightPosition;
  uniformsScene.u_pointLightColor = config_.pointLightColor;

  uniformBlockScene_->setData(&uniformsScene, sizeof(UniformsScene));
}

void Viewer::updateUniformModel(const glm::mat4 &model, const glm::mat4 &view) {
  static UniformsModel uniformsModel{};

  uniformsModel.u_reverseZ = config_.reverseZ ? 1u : 0u;
  uniformsModel.u_modelMatrix = model;
  uniformsModel.u_modelViewProjectionMatrix = camera_->projectionMatrix() * view * model;
  uniformsModel.u_inverseTransposeModelMatrix = glm::mat3(glm::transpose(glm::inverse(model)));

  // shadow mvp
  if (config_.shadowMap && cameraDepth_) {
    const glm::mat4 biasMat = glm::mat4(0.5f, 0.0f, 0.0f, 0.0f,
                                        0.0f, 0.5f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 1.0f, 0.0f,
                                        0.5f, 0.5f, 0.0f, 1.0f);
    uniformsModel.u_shadowMVPMatrix = biasMat * cameraDepth_->projectionMatrix() * cameraDepth_->viewMatrix() * model;
  }

  uniformBlockModel_->setData(&uniformsModel, sizeof(UniformsModel));
}

void Viewer::updateUniformMaterial(Material &material, float specular) {
  static UniformsMaterial uniformsMaterial{};

  uniformsMaterial.u_enableLight = config_.showLight ? 1u : 0u;
  uniformsMaterial.u_enableIBL = iBLEnabled() ? 1u : 0u;
  uniformsMaterial.u_enableShadow = config_.shadowMap ? 1u : 0u;

  uniformsMaterial.u_pointSize = material.pointSize;
  uniformsMaterial.u_kSpecular = specular;
  uniformsMaterial.u_baseColor = material.baseColor;

  uniformBlockMaterial_->setData(&uniformsMaterial, sizeof(UniformsMaterial));
}

bool Viewer::initSkyboxIBL() {
  if (!(config_.showSkybox && config_.pbrIbl)) {
    return false;
  }

  if (getSkyboxMaterial()->iblReady) {
    return true;
  }

  auto &skybox = scene_->skybox;
  if (skybox.material->textures.empty()) {
    setupTextures(*skybox.material);
  }

  std::shared_ptr<Texture> textureCube = nullptr;

  // convert equirectangular to cube map if needed
  auto texCubeIt = skybox.material->textures.find(MaterialTexType_CUBE);
  if (texCubeIt == skybox.material->textures.end()) {
    auto texEqIt = skybox.material->textures.find(MaterialTexType_EQUIRECTANGULAR);
    if (texEqIt != skybox.material->textures.end()) {
      auto tex2d = std::dynamic_pointer_cast<Texture>(texEqIt->second);
      auto cubeSize = std::min(tex2d->width, tex2d->height);
      auto texCvt = createTextureCubeDefault(cubeSize, cubeSize, TextureUsage_AttachmentColor | TextureUsage_Sampler);
      auto success = Environment::convertEquirectangular(renderer_,
                                                         [&](ShaderProgram &program) -> bool {
                                                           return loadShaders(program, Shading_Skybox);
                                                         },
                                                         tex2d,
                                                         texCvt);

      LOGD("convert equirectangular to cube map: %s.", success ? "success" : "failed");
      if (success) {
        textureCube = texCvt;
        skybox.material->textures[MaterialTexType_CUBE] = texCvt;

        // update skybox material
        skybox.material->textures.erase(MaterialTexType_EQUIRECTANGULAR);
        skybox.material->shaderDefines = generateShaderDefines(*skybox.material);
        skybox.material->materialObj = nullptr;
      }
    }
  } else {
    textureCube = std::dynamic_pointer_cast<Texture>(texCubeIt->second);
  }

  if (!textureCube) {
    LOGE("initSkyboxIBL failed: skybox texture cube not available");
    return false;
  }

  // generate irradiance map
  LOGD("generate ibl irradiance map ...");
  auto texIrradiance = createTextureCubeDefault(kIrradianceMapSize, kIrradianceMapSize, TextureUsage_AttachmentColor | TextureUsage_Sampler);
  if (Environment::generateIrradianceMap(renderer_,
                                         [&](ShaderProgram &program) -> bool {
                                           return loadShaders(program, Shading_IBL_Irradiance);
                                         },
                                         textureCube,
                                         texIrradiance)) {
    skybox.material->textures[MaterialTexType_IBL_IRRADIANCE] = std::move(texIrradiance);
  } else {
    LOGE("initSkyboxIBL failed: generate irradiance map failed");
    return false;
  }
  LOGD("generate ibl irradiance map done.");

  // generate prefilter map
  LOGD("generate ibl prefilter map ...");
  auto texPrefilter = createTextureCubeDefault(kPrefilterMapSize, kPrefilterMapSize, TextureUsage_AttachmentColor | TextureUsage_Sampler, true);
  if (Environment::generatePrefilterMap(renderer_,
                                        [&](ShaderProgram &program) -> bool {
                                          return loadShaders(program, Shading_IBL_Prefilter);
                                        },
                                        textureCube,
                                        texPrefilter)) {
    skybox.material->textures[MaterialTexType_IBL_PREFILTER] = std::move(texPrefilter);
  } else {
    LOGE("initSkyboxIBL failed: generate prefilter map failed");
    return false;
  }
  LOGD("generate ibl prefilter map done.");

  getSkyboxMaterial()->iblReady = true;
  return true;
}

bool Viewer::iBLEnabled() {
  return config_.showSkybox && config_.pbrIbl && getSkyboxMaterial()->iblReady;
}

SkyboxMaterial *Viewer::getSkyboxMaterial() {
  return dynamic_cast<SkyboxMaterial *>(scene_->skybox.material.get());
}

void Viewer::updateIBLTextures(MaterialObject *materialObj) {
  if (!materialObj->shaderResources) {
    return;
  }

  auto &samplers = materialObj->shaderResources->samplers;
  if (iBLEnabled()) {
    auto &skyboxTextures = scene_->skybox.material->textures;
    samplers[MaterialTexType_IBL_IRRADIANCE]->setTexture(skyboxTextures[MaterialTexType_IBL_IRRADIANCE]);
    samplers[MaterialTexType_IBL_PREFILTER]->setTexture(skyboxTextures[MaterialTexType_IBL_PREFILTER]);
  } else {
    samplers[MaterialTexType_IBL_IRRADIANCE]->setTexture(iblPlaceholder_);
    samplers[MaterialTexType_IBL_PREFILTER]->setTexture(iblPlaceholder_);
  }
}

void Viewer::updateShadowTextures(MaterialObject *materialObj) {
  if (!materialObj->shaderResources) {
    return;
  }

  auto &samplers = materialObj->shaderResources->samplers;
  if (config_.shadowMap) {
    samplers[MaterialTexType_SHADOWMAP]->setTexture(texDepthShadow_);
  } else {
    samplers[MaterialTexType_SHADOWMAP]->setTexture(shadowPlaceholder_);
  }
}

std::shared_ptr<Texture> Viewer::createTextureCubeDefault(int width, int height, uint32_t usage, bool mipmaps) {
  TextureDesc texDesc{};
  texDesc.width = width;
  texDesc.height = height;
  texDesc.type = TextureType_CUBE;
  texDesc.format = TextureFormat_RGBA8;
  texDesc.usage = usage;
  texDesc.useMipmaps = mipmaps;
  texDesc.multiSample = false;
  auto textureCube = renderer_->createTexture(texDesc);
  if (!textureCube) {
    return nullptr;
  }

  SamplerDesc sampler{};
  sampler.filterMin = mipmaps ? Filter_LINEAR_MIPMAP_LINEAR : Filter_LINEAR;
  sampler.filterMag = Filter_LINEAR;
  textureCube->setSamplerDesc(sampler);

  textureCube->initImageData();
  return textureCube;
}

std::shared_ptr<Texture> Viewer::createTexture2DDefault(int width, int height, TextureFormat format, uint32_t usage, bool mipmaps) {
  TextureDesc texDesc{};
  texDesc.width = width;
  texDesc.height = height;
  texDesc.type = TextureType_2D;
  texDesc.format = format;
  texDesc.usage = usage;
  texDesc.useMipmaps = mipmaps;
  texDesc.multiSample = false;
  auto texture2d = renderer_->createTexture(texDesc);
  if (!texture2d) {
    return nullptr;
  }

  SamplerDesc sampler{};
  sampler.filterMin = mipmaps ? Filter_LINEAR_MIPMAP_LINEAR : Filter_LINEAR;
  sampler.filterMag = Filter_LINEAR;
  texture2d->setSamplerDesc(sampler);

  texture2d->initImageData();
  return texture2d;
}

std::set<std::string> Viewer::generateShaderDefines(Material &material) {
  std::set<std::string> shaderDefines;
  for (auto &kv : material.textures) {
    const char *samplerDefine = Material::samplerDefine((MaterialTexType) kv.first);
    if (samplerDefine) {
      shaderDefines.insert(samplerDefine);
    }
  }
  return shaderDefines;
}

size_t Viewer::getShaderProgramCacheKey(ShadingModel shading, const std::set<std::string> &defines) {
  size_t seed = 0;
  HashUtils::hashCombine(seed, (int) shading);
  for (auto &def : defines) {
    HashUtils::hashCombine(seed, def);
  }
  return seed;
}

size_t Viewer::getPipelineCacheKey(Material &material, const RenderStates &rs) {
  size_t seed = 0;

  HashUtils::hashCombine(seed, (int) material.materialObj->shadingModel);

  // TODO pack together
  HashUtils::hashCombine(seed, rs.blend);
  HashUtils::hashCombine(seed, (int) rs.blendParams.blendFuncRgb);
  HashUtils::hashCombine(seed, (int) rs.blendParams.blendSrcRgb);
  HashUtils::hashCombine(seed, (int) rs.blendParams.blendDstRgb);
  HashUtils::hashCombine(seed, (int) rs.blendParams.blendFuncAlpha);
  HashUtils::hashCombine(seed, (int) rs.blendParams.blendSrcAlpha);
  HashUtils::hashCombine(seed, (int) rs.blendParams.blendDstAlpha);

  HashUtils::hashCombine(seed, rs.depthTest);
  HashUtils::hashCombine(seed, rs.depthMask);
  HashUtils::hashCombine(seed, (int) rs.depthFunc);

  HashUtils::hashCombine(seed, rs.cullFace);
  HashUtils::hashCombine(seed, (int) rs.primitiveType);
  HashUtils::hashCombine(seed, (int) rs.polygonMode);

  HashUtils::hashCombine(seed, rs.lineWidth);

  return seed;
}

bool Viewer::checkMeshFrustumCull(ModelMesh &mesh, const glm::mat4 &transform) {
  BoundingBox bbox = mesh.aabb.transform(transform);
  return camera_->getFrustum().intersects(bbox);
}

}
}
