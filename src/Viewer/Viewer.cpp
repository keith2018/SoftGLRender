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

void Viewer::create(int width, int height, int outTexId) {
  width_ = width;
  height_ = height;
  outTexId_ = outTexId;

  // create renderer
  renderer_ = createRenderer();

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
  shadowPlaceholder_ = createTexture2DDefault(1, 1, TextureFormat_DEPTH);
  fxaaFilter_ = nullptr;
  texColorFxaa_ = nullptr;
  iblPlaceholder_ = createTextureCubeDefault(1, 1);
  programCache_.clear();
}

void Viewer::configRenderer() {}

void Viewer::drawFrame(DemoScene &scene) {
  scene_ = &scene;

  // init frame buffers
  setupMainBuffers();

  // FXAA
  if (config_.aaType == AAType_FXAA) {
    processFXAASetup();
  }

  // set fbo & viewport
  renderer_->setFrameBuffer(fboMain_);
  renderer_->setViewPort(0, 0, width_, height_);

  // clear
  ClearState clearState;
  clearState.clearColor = config_.clearColor;
  renderer_->clear(clearState);

  // update scene uniform
  updateUniformScene();

  // init skybox ibl
  if (config_.showSkybox && config_.pbrIbl) {
    initSkyboxIBL(scene_->skybox);
  }

  // draw shadow map
  if (config_.shadowMap) {
    // set fbo & viewport
    setupShowMapBuffers();
    renderer_->setFrameBuffer(fboShadow_);
    renderer_->setViewPort(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

    // clear
    ClearState clearDepth;
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

void Viewer::destroy() {}

void Viewer::processFXAASetup() {
  if (!texColorFxaa_) {
    Sampler2DDesc sampler;
    sampler.useMipmaps = false;
    sampler.filterMin = Filter_LINEAR;

    texColorFxaa_ = renderer_->createTexture({TextureType_2D, TextureFormat_RGBA8, false});
    texColorFxaa_->setSamplerDesc(sampler);
    texColorFxaa_->initImageData(width_, height_);
  }

  if (!fxaaFilter_) {
    fxaaFilter_ = std::make_shared<QuadFilter>(createRenderer(),
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
  updateUniformMaterial(points.material.baseColor);

  pipelineSetup(points,
                points.material,
                {
                    {UniformBlock_Model, uniformsBlockModel_},
                    {UniformBlock_Material, uniformsBlockMaterial_},
                },
                [&](RenderState &rs) -> void {
                  rs.pointSize = points.pointSize;
                });
  pipelineDraw(points, points.material);
}

void Viewer::drawLines(ModelLines &lines, glm::mat4 &transform) {
  updateUniformModel(transform, camera_->viewMatrix(), camera_->projectionMatrix());
  updateUniformMaterial(lines.material.baseColor);

  pipelineSetup(lines,
                lines.material,
                {
                    {UniformBlock_Model, uniformsBlockModel_},
                    {UniformBlock_Material, uniformsBlockMaterial_},
                },
                [&](RenderState &rs) -> void {
                  rs.lineWidth = lines.lineWidth;
                });
  pipelineDraw(lines, lines.material);
}

void Viewer::drawSkybox(ModelSkybox &skybox, glm::mat4 &transform) {
  updateUniformModel(transform, glm::mat3(camera_->viewMatrix()), camera_->projectionMatrix());

  pipelineSetup(skybox,
                *skybox.material,
                {
                    {UniformBlock_Model, uniformsBlockModel_}
                },
                [&](RenderState &rs) -> void {
                  rs.depthFunc = config_.reverseZ ? DepthFunc_GEQUAL : DepthFunc_LEQUAL;
                  rs.depthMask = false;
                });
  pipelineDraw(skybox, *skybox.material);
}

void Viewer::drawMeshBaseColor(ModelMesh &mesh, bool wireframe) {
  updateUniformMaterial(mesh.materialBaseColor.baseColor);

  pipelineSetup(mesh,
                mesh.materialBaseColor,
                {
                    {UniformBlock_Model, uniformsBlockModel_},
                    {UniformBlock_Scene, uniformBlockScene_},
                    {UniformBlock_Material, uniformsBlockMaterial_},
                },
                [&](RenderState &rs) -> void {
                  rs.polygonMode = wireframe ? PolygonMode_LINE : PolygonMode_FILL;
                });
  pipelineDraw(mesh, mesh.materialBaseColor);
}

void Viewer::drawMeshTextured(ModelMesh &mesh, float specular) {
  updateUniformMaterial(glm::vec4(1.f), specular);

  pipelineSetup(mesh,
                mesh.materialTextured,
                {
                    {UniformBlock_Model, uniformsBlockModel_},
                    {UniformBlock_Scene, uniformBlockScene_},
                    {UniformBlock_Material, uniformsBlockMaterial_},
                });

  // update IBL textures
  if (mesh.materialTextured.shading == Shading_PBR) {
    updateIBLTextures(mesh.materialTextured);
  }

  // update shadow textures
  if (config_.shadowMap) {
    updateShadowTextures(mesh.materialTextured);
  }

  pipelineDraw(mesh, mesh.materialTextured);
}

void Viewer::drawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe) {
  // update mvp uniform
  glm::mat4 modelMatrix = transform * node.transform;
  updateUniformModel(modelMatrix, camera_->viewMatrix(), camera_->projectionMatrix());

  // draw nodes
  for (auto &mesh : node.meshes) {
    if (mesh.materialTextured.alphaMode != mode) {
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

void Viewer::pipelineSetup(ModelVertexes &vertexes, Material &material,
                           const std::unordered_map<int, std::shared_ptr<UniformBlock>> &uniformBlocks,
                           const std::function<void(RenderState &rs)> &extraStates) {
  setupVertexArray(vertexes);
  setupRenderStates(material.renderState, material.alphaMode == Alpha_Blend);
  material.renderState.cullFace = config_.cullFace && (!material.doubleSided);
  if (extraStates) {
    extraStates(material.renderState);
  }
  setupMaterial(material, uniformBlocks);
}

void Viewer::pipelineDraw(ModelVertexes &vertexes, Material &material) {
  renderer_->setVertexArrayObject(vertexes.vao);
  renderer_->setRenderState(material.renderState);
  renderer_->setShaderProgram(material.shaderProgram);
  renderer_->setShaderUniforms(material.shaderUniforms);
  renderer_->draw(vertexes.primitiveType);
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

void Viewer::setupShowMapBuffers() {
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
    texDepthShadow_ = renderer_->createTexture({TextureType_2D, TextureFormat_DEPTH, false});
    texDepthShadow_->setSamplerDesc(sampler);
    texDepthShadow_->initImageData(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

    fboShadow_->setDepthAttachment(texDepthShadow_);

    if (!fboShadow_->isValid()) {
      LOGE("setupShowMapBuffers failed");
    }
  }
}

void Viewer::setupMainColorBuffer(bool multiSample) {
  if (!texColorMain_ || texColorMain_->multiSample != multiSample) {
    Sampler2DDesc sampler;
    sampler.useMipmaps = false;
    sampler.filterMin = Filter_LINEAR;
    texColorMain_ = renderer_->createTexture({TextureType_2D, TextureFormat_RGBA8, multiSample});
    texColorMain_->setSamplerDesc(sampler);
    texColorMain_->initImageData(width_, height_);
  }
}

void Viewer::setupMainDepthBuffer(bool multiSample) {
  if (!texDepthMain_ || texDepthMain_->multiSample != multiSample) {
    Sampler2DDesc sampler;
    sampler.useMipmaps = false;
    sampler.filterMin = Filter_NEAREST;
    sampler.filterMag = Filter_NEAREST;
    texDepthMain_ = renderer_->createTexture({TextureType_2D, TextureFormat_DEPTH, multiSample});
    texDepthMain_->setSamplerDesc(sampler);
    texDepthMain_->initImageData(width_, height_);
  }
}

void Viewer::setupVertexArray(ModelVertexes &vertexes) {
  if (!vertexes.vao) {
    vertexes.vao = renderer_->createVertexArrayObject(vertexes);
  }
}

void Viewer::setupRenderStates(RenderState &rs, bool blend) const {
  rs.blend = blend;
  rs.blendParams.SetBlendFactor(BlendFactor_SRC_ALPHA, BlendFactor_ONE_MINUS_SRC_ALPHA);

  rs.depthTest = config_.depthTest;
  rs.depthMask = !blend;  // disable depth write
  rs.depthFunc = config_.reverseZ ? DepthFunc_GEQUAL : DepthFunc_LESS;

  rs.cullFace = config_.cullFace;
  rs.polygonMode = PolygonMode_FILL;

  rs.lineWidth = 1.f;
  rs.pointSize = 1.f;
}

void Viewer::setupTextures(Material &material) {
  for (auto &kv : material.textureData) {
    std::shared_ptr<Texture> texture = nullptr;
    switch (kv.first) {
      case TextureUsage_IBL_IRRADIANCE:
      case TextureUsage_IBL_PREFILTER: {
        // skip ibl textures
        break;
      }
      case TextureUsage_CUBE: {
        texture = renderer_->createTexture({TextureType_CUBE, TextureFormat_RGBA8, false});
        SamplerCubeDesc samplerCube;
        samplerCube.wrapS = kv.second.wrapMode;
        samplerCube.wrapT = kv.second.wrapMode;
        samplerCube.wrapR = kv.second.wrapMode;
        texture->setSamplerDesc(samplerCube);
        break;
      }
      default: {
        texture = renderer_->createTexture({TextureType_2D, TextureFormat_RGBA8, false});
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
  material.textures[TextureUsage_SHADOWMAP] = shadowPlaceholder_;

  // default IBL texture
  if (material.shading == Shading_PBR) {
    material.textures[TextureUsage_IBL_IRRADIANCE] = iblPlaceholder_;
    material.textures[TextureUsage_IBL_PREFILTER] = iblPlaceholder_;
  }
}

void Viewer::setupSamplerUniforms(Material &material) {
  for (auto &kv : material.textures) {
    // create sampler uniform
    const char *samplerName = Material::samplerName((TextureUsage) kv.first);
    if (samplerName) {
      auto uniform = renderer_->createUniformSampler(samplerName, kv.second->type, kv.second->format);
      uniform->setTexture(kv.second);
      material.shaderUniforms->samplers[kv.first] = std::move(uniform);
    }
  }
}

bool Viewer::setupShaderProgram(Material &material) {
  auto shaderDefines = generateShaderDefines(material);
  size_t cacheKey = getShaderProgramCacheKey(material.shading, shaderDefines);

  // try cache
  auto cachedProgram = programCache_.find(cacheKey);
  if (cachedProgram != programCache_.end()) {
    material.shaderProgram = cachedProgram->second;
    material.shaderUniforms = std::make_shared<ShaderUniforms>();
    return true;
  }

  auto program = renderer_->createShaderProgram();
  program->addDefines(shaderDefines);

  bool success = loadShaders(*program, material.shading);
  if (success) {
    // add to cache
    programCache_[cacheKey] = program;
    material.shaderProgram = program;
    material.shaderUniforms = std::make_shared<ShaderUniforms>();
  } else {
    LOGE("setupShaderProgram failed: %s", Material::shadingModelStr(material.shading));
  }

  return success;
}

void Viewer::setupMaterial(Material &material,
                           const std::unordered_map<int, std::shared_ptr<UniformBlock>> &uniformBlocks) {
  material.createTextures([&]() -> void {
    setupTextures(material);
  });

  material.createProgram([&]() -> void {
    if (setupShaderProgram(material)) {
      setupSamplerUniforms(material);
      for (auto &kv : uniformBlocks) {
        material.shaderUniforms->blocks.insert(kv);
      }
    }
  });
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
  if (skybox.material->iblReady) {
    return true;
  }

  if (skybox.material->iblError) {
    return false;
  }

  skybox.material->createTextures([&]() -> void {
    setupTextures(*skybox.material);
  });

  std::shared_ptr<Texture> textureCube = nullptr;

  // convert equirectangular to cube map if needed
  auto texCubeIt = skybox.material->textures.find(TextureUsage_CUBE);
  if (texCubeIt == skybox.material->textures.end()) {
    auto texEqIt = skybox.material->textures.find(TextureUsage_EQUIRECTANGULAR);
    if (texEqIt != skybox.material->textures.end()) {
      auto tex2d = std::dynamic_pointer_cast<Texture>(texEqIt->second);
      auto cubeSize = std::min(tex2d->width, tex2d->height);
      auto texCvt = createTextureCubeDefault(cubeSize, cubeSize);
      auto success = Environment::convertEquirectangular(createRenderer(),
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
        skybox.material->resetProgram();
      }
    }
  } else {
    textureCube = std::dynamic_pointer_cast<Texture>(texCubeIt->second);
  }

  if (!textureCube) {
    LOGE("initSkyboxIBL failed: skybox texture cube not available");
    skybox.material->iblError = true;
    return false;
  }

  // generate irradiance map
  LOGD("generate ibl irradiance map ...");
  auto texIrradiance = createTextureCubeDefault(kIrradianceMapSize, kIrradianceMapSize);
  if (Environment::generateIrradianceMap(createRenderer(),
                                         [&](ShaderProgram &program) -> bool {
                                           return loadShaders(program, Shading_IBL_Irradiance);
                                         },
                                         textureCube,
                                         texIrradiance)) {
    skybox.material->textures[TextureUsage_IBL_IRRADIANCE] = std::move(texIrradiance);
  } else {
    LOGE("initSkyboxIBL failed: generate irradiance map failed");
    skybox.material->iblError = true;
    return false;
  }
  LOGD("generate ibl irradiance map done.");

  // generate prefilter map
  LOGD("generate ibl prefilter map ...");
  auto texPrefilter = createTextureCubeDefault(kPrefilterMapSize, kPrefilterMapSize, true);
  if (Environment::generatePrefilterMap(createRenderer(),
                                        [&](ShaderProgram &program) -> bool {
                                          return loadShaders(program, Shading_IBL_Prefilter);
                                        },
                                        textureCube,
                                        texPrefilter)) {
    skybox.material->textures[TextureUsage_IBL_PREFILTER] = std::move(texPrefilter);
  } else {
    LOGE("initSkyboxIBL failed: generate prefilter map failed");
    skybox.material->iblError = true;
    return false;
  }
  LOGD("generate ibl prefilter map done.");

  skybox.material->iblReady = true;
  return true;
}

bool Viewer::iBLEnabled() {
  return config_.showSkybox && config_.pbrIbl && scene_->skybox.material->iblReady;
}

void Viewer::updateIBLTextures(Material &material) {
  if (!material.shaderUniforms) {
    return;
  }

  auto &samplers = material.shaderUniforms->samplers;
  if (iBLEnabled()) {
    auto &skyboxTextures = scene_->skybox.material->textures;
    samplers[TextureUsage_IBL_IRRADIANCE]->setTexture(skyboxTextures[TextureUsage_IBL_IRRADIANCE]);
    samplers[TextureUsage_IBL_PREFILTER]->setTexture(skyboxTextures[TextureUsage_IBL_PREFILTER]);
  } else {
    samplers[TextureUsage_IBL_IRRADIANCE]->setTexture(iblPlaceholder_);
    samplers[TextureUsage_IBL_PREFILTER]->setTexture(iblPlaceholder_);
  }
}

void Viewer::updateShadowTextures(Material &material) {
  if (!material.shaderUniforms) {
    return;
  }

  auto &samplers = material.shaderUniforms->samplers;
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

  auto textureCube = renderer_->createTexture({TextureType_CUBE, TextureFormat_RGBA8, false});
  textureCube->setSamplerDesc(samplerCube);
  textureCube->initImageData(width, height);

  return textureCube;
}

std::shared_ptr<Texture> Viewer::createTexture2DDefault(int width, int height, TextureFormat format, bool mipmaps) {
  Sampler2DDesc sampler2d;
  sampler2d.useMipmaps = mipmaps;
  sampler2d.filterMin = mipmaps ? Filter_LINEAR_MIPMAP_LINEAR : Filter_LINEAR;

  auto texture2d = renderer_->createTexture({TextureType_2D, format, false});
  texture2d->setSamplerDesc(sampler2d);
  texture2d->initImageData(width, height);

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

bool Viewer::checkMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform) {
  BoundingBox bbox = mesh.aabb.transform(transform);
  return camera_->getFrustum().intersects(bbox);
}

}
}
