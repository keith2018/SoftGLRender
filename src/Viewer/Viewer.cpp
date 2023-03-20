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

bool Viewer::create(int width, int height, int outTexId) {
  width_ = width;
  height_ = height;
  outTexId_ = outTexId;

  // create renderer
  if (!renderer_) {
    renderer_ = createRenderer();
  }
  if (!renderer_) {
    return false;
  }

  // create uniforms
  uniformBlockScene_ = renderer_->createUniformBlock("UniformsScene", sizeof(UniformsScene));
  uniformsBlockModel_ = renderer_->createUniformBlock("UniformsModel", sizeof(UniformsModel));
  uniformsBlockMaterial_ = renderer_->createUniformBlock("UniformsMaterial", sizeof(UniformsMaterial));

  // reset variables
  camera_ = &cameraMain_;
  fboMain_ = nullptr;
  texColorMain_ = nullptr;
  texDepthMain_ = nullptr;
  fboShadow_ = nullptr;
  texDepthShadow_ = nullptr;
  shadowPlaceholder_ = createTexture2DDefault(1, 1, TextureFormat_FLOAT32);
  fxaaFilter_ = nullptr;
  texColorFxaa_ = nullptr;
  iblPlaceholder_ = createTextureCubeDefault(1, 1);
  programCache_.clear();
  pipelineCache_.clear();

  return true;
}

void Viewer::destroy() {
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
  uniformsBlockModel_ = nullptr;
  uniformsBlockMaterial_ = nullptr;

  programCache_.clear();
  pipelineCache_.clear();
}

void Viewer::configRenderer() {}

void Viewer::drawFrame(DemoScene &scene) {
  if (!renderer_) {
    return;
  }

  scene_ = &scene;

  // init frame buffers
  setupMainBuffers();

  // FXAA
  if (config_.aaType == AAType_FXAA) {
    processFXAASetup();
  }

  // init skybox ibl
  if (config_.showSkybox && config_.pbrIbl) {
    initSkyboxIBL(scene_->skybox);
  }

  // set fbo & viewport
  renderer_->setFrameBuffer(fboMain_);
  renderer_->setViewPort(0, 0, width_, height_);

  // clear
  ClearStates clearStates;
  clearStates.clearColor = config_.clearColor;
  renderer_->clear(clearStates);

  // update scene uniform
  updateUniformScene();

  // draw shadow map
  if (config_.shadowMap) {
    // set fbo & viewport
    setupShadowMapBuffers();
    renderer_->setFrameBuffer(fboShadow_);
    renderer_->setViewPort(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

    // clear
    ClearStates clearDepth;
    clearDepth.depthFlag = true;
    clearDepth.colorFlag = false;
    renderer_->clear(clearDepth);

    // set camera
    if (!cameraDepth_) {
      cameraDepth_ = std::make_shared<Camera>();
      cameraDepth_->setPerspective(glm::radians(CAMERA_FOV),
                                   (float) SHADOW_MAP_WIDTH / (float) SHADOW_MAP_HEIGHT,
                                   CAMERA_NEAR,
                                   CAMERA_FAR);
    }
    cameraDepth_->lookAt(config_.pointLightPosition, glm::vec3(0), glm::vec3(0, 1, 0));
    cameraDepth_->update();
    camera_ = cameraDepth_.get();

    // draw scene
    drawScene(false, false);

    // set back to main camera
    camera_ = &cameraMain_;

    // set back to main fbo
    renderer_->setFrameBuffer(fboMain_);
    renderer_->setViewPort(0, 0, width_, height_);
  }

  // draw point light
  if (config_.showLight) {
    glm::mat4 lightTransform(1.0f);
    drawPoints(scene_->pointLight, lightTransform);
  }

  // draw world axis
  if (config_.worldAxis) {
    glm::mat4 axisTransform(1.0f);
    drawLines(scene_->worldAxis, axisTransform);
  }

  // draw scene
  drawScene(config_.showFloor, config_.showSkybox);

  // FXAA
  if (config_.aaType == AAType_FXAA) {
    processFXAADraw();
  }
}

void Viewer::processFXAASetup() {
  if (!texColorFxaa_) {
    Sampler2DDesc sampler;
    sampler.useMipmaps = false;
    sampler.filterMin = Filter_LINEAR;

    texColorFxaa_ = renderer_->createTexture({width_, height_, TextureType_2D, TextureFormat_RGBA8, false});
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
  fxaaFilter_->setTextures(texColorFxaa_, texColorMain_);
}

void Viewer::processFXAADraw() {
  fxaaFilter_->draw();
}

void Viewer::drawScene(bool floor, bool skybox) {
  // draw floor
  if (floor) {
    glm::mat4 floorMatrix(1.0f);
    updateUniformModel(floorMatrix, camera_->viewMatrix(), camera_->projectionMatrix());
    if (config_.wireframe) {
      drawMeshBaseColor(scene_->floor, true);
    } else {
      drawMeshTextured(scene_->floor, 0.f);
    }
  }

  // draw model nodes opaque
  ModelNode &modelNode = scene_->model->rootNode;
  drawModelNodes(modelNode, scene_->model->centeredTransform, Alpha_Opaque, config_.wireframe);

  // draw skybox
  if (skybox) {
    glm::mat4 skyboxTransform(1.f);
    drawSkybox(scene_->skybox, skyboxTransform);
  }

  // draw model nodes blend
  drawModelNodes(modelNode, scene_->model->centeredTransform, Alpha_Blend, config_.wireframe);
}

void Viewer::drawPoints(ModelPoints &points, glm::mat4 &transform) {
  updateUniformModel(transform, camera_->viewMatrix(), camera_->projectionMatrix());
  updateUniformMaterial(points.material->baseColor);

  pipelineSetup(points,
                points.material->shadingModel,
                {
                    {UniformBlock_Model, uniformsBlockModel_},
                    {UniformBlock_Material, uniformsBlockMaterial_},
                });
  pipelineDraw(points);
}

void Viewer::drawLines(ModelLines &lines, glm::mat4 &transform) {
  updateUniformModel(transform, camera_->viewMatrix(), camera_->projectionMatrix());
  updateUniformMaterial(lines.material->baseColor);

  pipelineSetup(lines,
                lines.material->shadingModel,
                {
                    {UniformBlock_Model, uniformsBlockModel_},
                    {UniformBlock_Material, uniformsBlockMaterial_},
                });
  pipelineDraw(lines);
}

void Viewer::drawSkybox(ModelSkybox &skybox, glm::mat4 &transform) {
  updateUniformModel(transform, glm::mat3(camera_->viewMatrix()), camera_->projectionMatrix());

  pipelineSetup(skybox,
                skybox.material->shadingModel,
                {
                    {UniformBlock_Model, uniformsBlockModel_}
                },
                [&](RenderStates &rs) -> void {
                  rs.depthFunc = config_.reverseZ ? DepthFunc_GEQUAL : DepthFunc_LEQUAL;
                  rs.depthMask = false;
                });
  pipelineDraw(skybox);
}

void Viewer::drawMeshBaseColor(ModelMesh &mesh, bool wireframe) {
  updateUniformMaterial(mesh.material->baseColor);

  pipelineSetup(mesh,
                Shading_BaseColor,
                {
                    {UniformBlock_Model, uniformsBlockModel_},
                    {UniformBlock_Scene, uniformBlockScene_},
                    {UniformBlock_Material, uniformsBlockMaterial_},
                },
                [&](RenderStates &rs) -> void {
                  rs.polygonMode = wireframe ? PolygonMode_LINE : PolygonMode_FILL;
                });
  pipelineDraw(mesh);
}

void Viewer::drawMeshTextured(ModelMesh &mesh, float specular) {
  updateUniformMaterial(glm::vec4(1.f), specular);

  pipelineSetup(mesh,
                mesh.material->shadingModel,
                {
                    {UniformBlock_Model, uniformsBlockModel_},
                    {UniformBlock_Scene, uniformBlockScene_},
                    {UniformBlock_Material, uniformsBlockMaterial_},
                });

  // update IBL textures
  if (mesh.material->shadingModel == Shading_PBR) {
    updateIBLTextures(mesh.material->materialObj.get());
  }

  // update shadow textures
  if (config_.shadowMap) {
    updateShadowTextures(mesh.material->materialObj.get());
  }

  pipelineDraw(mesh);
}

void Viewer::drawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe) {
  // update mvp uniform
  glm::mat4 modelMatrix = transform * node.transform;
  updateUniformModel(modelMatrix, camera_->viewMatrix(), camera_->projectionMatrix());

  // draw nodes
  for (auto &mesh : node.meshes) {
    if (mesh.material->alphaMode != mode) {
      continue;
    }

    // frustum cull
    bool meshInside = checkMeshFrustumCull(mesh, modelMatrix);
    if (!meshInside) {
      continue;
    }

    // draw mesh
    wireframe ? drawMeshBaseColor(mesh, true) : drawMeshTextured(mesh, 1.f);
  }

  // draw child
  for (auto &child_node : node.children) {
    drawModelNodes(child_node, modelMatrix, mode, wireframe);
  }
}

void Viewer::pipelineSetup(ModelBase &model, ShadingModel shading,
                           const std::unordered_map<int, std::shared_ptr<UniformBlock>> &uniformBlocks,
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
    fboMain_ = renderer_->createFrameBuffer();
  }
  fboMain_->setColorAttachment(texColorMain_, 0);
  fboMain_->setDepthAttachment(texDepthMain_);

  if (!fboMain_->isValid()) {
    LOGE("setupMainBuffers failed");
  }
}

void Viewer::setupShadowMapBuffers() {
  if (!fboShadow_) {
    fboShadow_ = renderer_->createFrameBuffer();
  }

  if (!texDepthShadow_) {
    Sampler2DDesc sampler;
    sampler.useMipmaps = false;
    sampler.filterMin = Filter_NEAREST;
    sampler.filterMag = Filter_NEAREST;
    sampler.wrapS = Wrap_CLAMP_TO_BORDER;
    sampler.wrapT = Wrap_CLAMP_TO_BORDER;
    sampler.borderColor = glm::vec4(config_.reverseZ ? 0.f : 1.f);
    texDepthShadow_ = renderer_->createTexture(
        {SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, TextureType_2D, TextureFormat_FLOAT32, false});
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
    Sampler2DDesc sampler;
    sampler.useMipmaps = false;
    sampler.filterMin = Filter_LINEAR;
    texColorMain_ = renderer_->createTexture({width_, height_, TextureType_2D, TextureFormat_RGBA8, multiSample});
    texColorMain_->setSamplerDesc(sampler);
    texColorMain_->initImageData();
  }
}

void Viewer::setupMainDepthBuffer(bool multiSample) {
  if (!texDepthMain_ || texDepthMain_->multiSample != multiSample) {
    Sampler2DDesc sampler;
    sampler.useMipmaps = false;
    sampler.filterMin = Filter_NEAREST;
    sampler.filterMag = Filter_NEAREST;
    texDepthMain_ = renderer_->createTexture({width_, height_, TextureType_2D, TextureFormat_FLOAT32, multiSample});
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
    int width = (int) kv.second.width;
    int height = (int) kv.second.height;
    std::shared_ptr<Texture> texture = nullptr;
    switch (kv.first) {
      case TextureUsage_IBL_IRRADIANCE:
      case TextureUsage_IBL_PREFILTER: {
        // skip ibl textures
        break;
      }
      case TextureUsage_CUBE: {
        texture = renderer_->createTexture({width, height, TextureType_CUBE, TextureFormat_RGBA8, false});
        SamplerCubeDesc samplerCube;
        samplerCube.wrapS = kv.second.wrapMode;
        samplerCube.wrapT = kv.second.wrapMode;
        samplerCube.wrapR = kv.second.wrapMode;
        texture->setSamplerDesc(samplerCube);
        break;
      }
      default: {
        texture = renderer_->createTexture({width, height, TextureType_2D, TextureFormat_RGBA8, false});
        Sampler2DDesc sampler2d;
        sampler2d.wrapS = kv.second.wrapMode;
        sampler2d.wrapT = kv.second.wrapMode;
        if (config_.mipmaps) {
          sampler2d.useMipmaps = true;
          sampler2d.filterMin = Filter_LINEAR_MIPMAP_LINEAR;
        }
        texture->setSamplerDesc(sampler2d);
        break;
      }
    }

    // upload image data
    texture->setImageData(kv.second.data);
    material.textures[kv.first] = texture;
  }

  // default shadow depth texture
  if (material.shadingModel != Shading_Skybox) {
    material.textures[TextureUsage_SHADOWMAP] = shadowPlaceholder_;
  }

  // default IBL texture
  if (material.shadingModel == Shading_PBR) {
    material.textures[TextureUsage_IBL_IRRADIANCE] = iblPlaceholder_;
    material.textures[TextureUsage_IBL_PREFILTER] = iblPlaceholder_;
  }
}

void Viewer::setupSamplerUniforms(Material &material) {
  for (auto &kv : material.textures) {
    // create sampler uniform
    const char *samplerName = Material::samplerName((TextureUsage) kv.first);
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
  rs.depthMask = !rs.blend;   // disable depth write
  rs.depthFunc = config_.reverseZ ? DepthFunc_GEQUAL : DepthFunc_LESS;

  rs.cullFace = config_.cullFace && (!material.doubleSided);
  rs.primitiveType = model.primitiveType;
  rs.polygonMode = PolygonMode_FILL;

  rs.lineWidth = material.lineWidth;
  rs.pointSize = material.pointSize;

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

void Viewer::setupMaterial(ModelBase &model, ShadingModel shading,
                           const std::unordered_map<int, std::shared_ptr<UniformBlock>> &uniformBlocks,
                           const std::function<void(RenderStates &rs)> &extraStates) {
  auto &material = *model.material;
  if (material.textures.empty()) {
    setupTextures(material);
    material.shaderDefines = generateShaderDefines(material);
  }

  if (!material.materialObj) {
    material.materialObj = std::make_shared<MaterialObject>();
    material.materialObj->shadingModel = shading;

    if (setupShaderProgram(material, shading)) {
      setupSamplerUniforms(material);
      for (auto &kv : uniformBlocks) {
        material.materialObj->shaderResources->blocks.insert(kv);
      }
    }
  }

  setupPipelineStates(model, extraStates);
}

void Viewer::updateUniformScene() {
  uniformsScene_.u_ambientColor = config_.ambientColor;
  uniformsScene_.u_cameraPosition = camera_->eye();
  uniformsScene_.u_pointLightPosition = config_.pointLightPosition;
  uniformsScene_.u_pointLightColor = config_.pointLightColor;
  uniformBlockScene_->setData(&uniformsScene_, sizeof(UniformsScene));
}

void Viewer::updateUniformModel(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &proj) {
  uniformsModel_.u_reverseZ = config_.reverseZ ? 1 : 0;

  uniformsModel_.u_modelMatrix = model;
  uniformsModel_.u_modelViewProjectionMatrix = proj * view * model;
  uniformsModel_.u_inverseTransposeModelMatrix = glm::mat3(glm::transpose(glm::inverse(model)));

  // shadow mvp
  if (config_.shadowMap && cameraDepth_) {
    uniformsModel_.u_shadowMVPMatrix = cameraDepth_->projectionMatrix() * cameraDepth_->viewMatrix() * model;
  }

  uniformsBlockModel_->setData(&uniformsModel_, sizeof(UniformsModel));
}

void Viewer::updateUniformMaterial(const glm::vec4 &color, float specular) {
  uniformsMaterial_.u_enableLight = config_.showLight ? 1 : 0;
  uniformsMaterial_.u_enableIBL = iBLEnabled() ? 1 : 0;
  uniformsMaterial_.u_enableShadow = config_.shadowMap;

  uniformsMaterial_.u_kSpecular = specular;
  uniformsMaterial_.u_baseColor = color;

  uniformsBlockMaterial_->setData(&uniformsMaterial_, sizeof(UniformsMaterial));
}

bool Viewer::initSkyboxIBL(ModelSkybox &skybox) {
  if (getSkyboxMaterial()->iblReady) {
    return true;
  }

  if (skybox.material->textures.empty()) {
    setupTextures(*skybox.material);
  }

  std::shared_ptr<Texture> textureCube = nullptr;

  // convert equirectangular to cube map if needed
  auto texCubeIt = skybox.material->textures.find(TextureUsage_CUBE);
  if (texCubeIt == skybox.material->textures.end()) {
    auto texEqIt = skybox.material->textures.find(TextureUsage_EQUIRECTANGULAR);
    if (texEqIt != skybox.material->textures.end()) {
      auto tex2d = std::dynamic_pointer_cast<Texture>(texEqIt->second);
      auto cubeSize = std::min(tex2d->width, tex2d->height);
      auto texCvt = createTextureCubeDefault(cubeSize, cubeSize);
      auto success = Environment::convertEquirectangular(renderer_,
                                                         [&](ShaderProgram &program) -> bool {
                                                           return loadShaders(program, Shading_Skybox);
                                                         },
                                                         tex2d,
                                                         texCvt);
      if (success) {
        textureCube = texCvt;
        skybox.material->textures[TextureUsage_CUBE] = texCvt;

        // update skybox material
        skybox.material->textures.erase(TextureUsage_EQUIRECTANGULAR);
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
  auto texIrradiance = createTextureCubeDefault(kIrradianceMapSize, kIrradianceMapSize);
  if (Environment::generateIrradianceMap(renderer_,
                                         [&](ShaderProgram &program) -> bool {
                                           return loadShaders(program, Shading_IBL_Irradiance);
                                         },
                                         textureCube,
                                         texIrradiance)) {
    skybox.material->textures[TextureUsage_IBL_IRRADIANCE] = std::move(texIrradiance);
  } else {
    LOGE("initSkyboxIBL failed: generate irradiance map failed");
    return false;
  }
  LOGD("generate ibl irradiance map done.");

  // generate prefilter map
  LOGD("generate ibl prefilter map ...");
  auto texPrefilter = createTextureCubeDefault(kPrefilterMapSize, kPrefilterMapSize, true);
  if (Environment::generatePrefilterMap(renderer_,
                                        [&](ShaderProgram &program) -> bool {
                                          return loadShaders(program, Shading_IBL_Prefilter);
                                        },
                                        textureCube,
                                        texPrefilter)) {
    skybox.material->textures[TextureUsage_IBL_PREFILTER] = std::move(texPrefilter);
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
    samplers[TextureUsage_IBL_IRRADIANCE]->setTexture(skyboxTextures[TextureUsage_IBL_IRRADIANCE]);
    samplers[TextureUsage_IBL_PREFILTER]->setTexture(skyboxTextures[TextureUsage_IBL_PREFILTER]);
  } else {
    samplers[TextureUsage_IBL_IRRADIANCE]->setTexture(iblPlaceholder_);
    samplers[TextureUsage_IBL_PREFILTER]->setTexture(iblPlaceholder_);
  }
}

void Viewer::updateShadowTextures(MaterialObject *materialObj) {
  if (!materialObj->shaderResources) {
    return;
  }

  auto &samplers = materialObj->shaderResources->samplers;
  if (config_.shadowMap) {
    samplers[TextureUsage_SHADOWMAP]->setTexture(texDepthShadow_);
  } else {
    samplers[TextureUsage_SHADOWMAP]->setTexture(shadowPlaceholder_);
  }
}

std::shared_ptr<Texture> Viewer::createTextureCubeDefault(int width, int height, bool mipmaps) {
  SamplerCubeDesc samplerCube;
  samplerCube.useMipmaps = mipmaps;
  samplerCube.filterMin = mipmaps ? Filter_LINEAR_MIPMAP_LINEAR : Filter_LINEAR;

  auto textureCube = renderer_->createTexture({width, height, TextureType_CUBE, TextureFormat_RGBA8, false});
  if (textureCube) {
    textureCube->setSamplerDesc(samplerCube);
    textureCube->initImageData();
  }
  return textureCube;
}

std::shared_ptr<Texture> Viewer::createTexture2DDefault(int width, int height, TextureFormat format, bool mipmaps) {
  Sampler2DDesc sampler2d;
  sampler2d.useMipmaps = mipmaps;
  sampler2d.filterMin = mipmaps ? Filter_LINEAR_MIPMAP_LINEAR : Filter_LINEAR;

  auto texture2d = renderer_->createTexture({width, height, TextureType_2D, format, false});
  if (texture2d) {
    texture2d->setSamplerDesc(sampler2d);
    texture2d->initImageData();
  }
  return texture2d;
}

std::set<std::string> Viewer::generateShaderDefines(Material &material) {
  std::set<std::string> shaderDefines;
  for (auto &kv : material.textures) {
    const char *samplerDefine = Material::samplerDefine((TextureUsage) kv.first);
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
  HashUtils::hashCombine(seed, rs.pointSize);

  return seed;
}

bool Viewer::checkMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform) {
  BoundingBox bbox = mesh.aabb.transform(transform);
  return camera_->getFrustum().intersects(bbox);
}

}
}
