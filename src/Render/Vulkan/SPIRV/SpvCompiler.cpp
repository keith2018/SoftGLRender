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
  explicit ReflectionTraverser(std::unordered_map<std::string, ShaderUniformDesc> &uniformsDesc)
      : uniformsDesc(uniformsDesc) {}

  void visitSymbol(glslang::TIntermSymbol *symbol) override {
    if (symbol->getQualifier().isUniform()) {
      auto accessName = symbol->getAccessName();
      auto qualifier = symbol->getQualifier();

      ShaderUniformDesc desc;
      desc.name = accessName.c_str();
      switch (symbol->getBasicType()) {
        case glslang::EbtSampler:
          desc.type = UniformType_Sampler;
          break;
        case glslang::EbtBlock:
          desc.type = UniformType_Block;
          break;
        default:
          desc.type = UniformType_Unknown;
          break;
      }

      desc.location = 0;
      desc.binding = 0;
      desc.set = 0;

      if (qualifier.hasLocation()) {
        desc.location = qualifier.layoutLocation;
      }
      if (qualifier.hasBinding()) {
        desc.binding = qualifier.layoutBinding;
      }
      if (qualifier.hasSet()) {
        desc.set = qualifier.layoutSet;
      }

      uniformsDesc[desc.name] = std::move(desc);
    }
  }

  std::unordered_map<std::string, ShaderUniformDesc> &uniformsDesc;
};

static ShaderCompilerResult compileShaderInternal(EShLanguage stage, const char *shaderSource) {
#ifdef DEBUG
  bool debug = true;
#else
  bool debug = false;
#endif

  ShaderCompilerResult compilerResult;

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
  shader.setAutoMapLocations(true);
  shader.setAutoMapBindings(true);

  // compile
  auto resources = GetDefaultResources();
  if (!shader.parse(resources, glslVersion, false, messages)) {
    LOGE("GLSL parsing failed:");
    LOGE("%s", shader.getInfoLog());
    LOGE("%s", shader.getInfoDebugLog());
    return {};
  }

  // reflect input & output
  ReflectionTraverser traverser(compilerResult.uniformsDesc);
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

  glslang::GlslangToSpv(*program.getIntermediate((EShLanguage) stage), compilerResult.spvCodes, &logger, &spvOptions);
  glslang::FinalizeProcess();

  return compilerResult;
}

ShaderCompilerResult SpvCompiler::compileVertexShader(const char *shaderSource) {
  return compileShaderInternal(EShLangVertex, shaderSource);
}

ShaderCompilerResult SpvCompiler::compileFragmentShader(const char *shaderSource) {
  return compileShaderInternal(EShLangFragment, shaderSource);
}

}
