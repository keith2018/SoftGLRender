/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "SpvCompiler.h"
#include "glslang/Public/ShaderLang.h"
#include "glslang/Public/ResourceLimits.h"
#include "SPIRV/GlslangToSpv.h"
#include "Base/Logger.h"

namespace SoftGL {

class ReflectionTraverser : public glslang::TIntermTraverser {
 public:
  void visitSymbol(glslang::TIntermSymbol *symbol) override {
    if (symbol->getQualifier().isUniformOrBuffer()) {
      auto accessName = symbol->getAccessName();
      auto qualifier = symbol->getQualifier();

      LOGD("%s:", accessName.c_str());
      LOGD("  type: %d", symbol->getBasicType());
      LOGD("  storage: %s", glslang::GetStorageQualifierString(qualifier.storage));
      if (qualifier.hasLocation()) {
        LOGD("  location: %d", qualifier.layoutLocation);
      }
      if (qualifier.hasBinding()) {
        LOGD("  binding: %d", qualifier.layoutBinding);
      }
      if (qualifier.hasSet()) {
        LOGD("  set: %d", qualifier.layoutSet);
      }
      if (qualifier.hasPacking()) {
        LOGD("  packing: %s", glslang::TQualifier::getLayoutPackingString(qualifier.layoutPacking));
      }
    }
  }
};

static std::vector<uint32_t> compileShaderInternal(EShLanguage stage, const char *shaderSource) {
#ifdef DEBUG
  bool debug = true;
#else
  bool debug = false;
#endif

  glslang::InitializeProcess();
  auto messages = (EShMessages) (EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault);

  glslang::TProgram program;
  glslang::TShader shader(stage);

  if (debug) {
    messages = (EShMessages) (messages | EShMsgDebugInfo);
  }

  const char *fileName = "";
  const int glslVersion = 450;
  shader.setStringsWithLengthsAndNames(&shaderSource, nullptr, &fileName, 1);
  shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, glslVersion);
  shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
  shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_0);

  // compile
  auto resources = GetDefaultResources();
  if (!shader.parse(resources, glslVersion, false, messages)) {
    LOGE("GLSL parsing failed:");
    LOGE("%s", shader.getInfoLog());
    LOGE("%s", shader.getInfoDebugLog());
    return {};
  }

  // reflect input & output
  ReflectionTraverser traverser;
  auto root = shader.getIntermediate()->getTreeRoot();
  root->traverse(&traverser);

  // link
  program.addShader(&shader);
  if (!program.link(messages)) {
    LOGE("GLSL linking failed:");
    LOGE("%s", program.getInfoLog());
    LOGE("%s", program.getInfoDebugLog());
    return {};
  }

  std::vector<uint32_t> spirv;
  spv::SpvBuildLogger logger;
  glslang::SpvOptions spvOptions;

  if (debug) {
    spvOptions.disableOptimizer = true;
    spvOptions.optimizeSize = false;
    spvOptions.generateDebugInfo = true;
  } else {
    spvOptions.generateDebugInfo = false;
    spvOptions.optimizeSize = true;
    spvOptions.disableOptimizer = false;
  }

  glslang::GlslangToSpv(*program.getIntermediate((EShLanguage) stage), spirv, &logger, &spvOptions);

  glslang::FinalizeProcess();
  return spirv;
}

std::vector<uint32_t> SpvCompiler::compileVertexShader(const char *shaderSource) {
  return compileShaderInternal(EShLangVertex, shaderSource);
}

std::vector<uint32_t> SpvCompiler::compileFragmentShader(const char *shaderSource) {
  return compileShaderInternal(EShLangFragment, shaderSource);
}

}
