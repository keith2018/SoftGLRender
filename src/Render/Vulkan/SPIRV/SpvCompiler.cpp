/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "SpvCompiler.h"
#include "Include/glslang_c_interface.h"
#include "Public/ResourceLimits.h"
#include "Base/Logger.h"

namespace SoftGL {

static std::vector<uint32_t> compileShaderInternal(glslang_stage_t stage, const char *shaderSource) {
  const glslang_input_t input = {
      .language = GLSLANG_SOURCE_GLSL,
      .stage = stage,
      .client = GLSLANG_CLIENT_VULKAN,
      .client_version = GLSLANG_TARGET_VULKAN_1_0,
      .target_language = GLSLANG_TARGET_SPV,
      .target_language_version = GLSLANG_TARGET_SPV_1_0,
      .code = shaderSource,
      .default_version = 450,
      .default_profile = GLSLANG_NO_PROFILE,
      .force_default_version_and_profile = false,
      .forward_compatible = false,
      .messages = GLSLANG_MSG_DEFAULT_BIT,
      .resource = reinterpret_cast<const glslang_resource_t *>(GetDefaultResources()),
  };

  glslang_shader_t *shader = glslang_shader_create(&input);

  if (!glslang_shader_preprocess(shader, &input)) {
    LOGE("GLSL preprocessing failed:");
    LOGE("%s", glslang_shader_get_info_log(shader));
    LOGE("%s", glslang_shader_get_info_debug_log(shader));
    LOGE("%s", input.code);
    glslang_shader_delete(shader);
    return {};
  }

  if (!glslang_shader_parse(shader, &input)) {
    LOGE("GLSL parsing failed:");
    LOGE("%s", glslang_shader_get_info_log(shader));
    LOGE("%s", glslang_shader_get_info_debug_log(shader));
    LOGE("%s", glslang_shader_get_preprocessed_code(shader));
    glslang_shader_delete(shader);
    return {};
  }

  glslang_program_t *program = glslang_program_create();
  glslang_program_add_shader(program, shader);

  if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
    LOGE("GLSL linking failed:");
    LOGE("%s", glslang_program_get_info_log(program));
    LOGE("%s", glslang_program_get_info_debug_log(program));
    glslang_program_delete(program);
    glslang_shader_delete(shader);
    return {};
  }

  glslang_program_SPIRV_generate(program, stage);

  std::vector<uint32_t> outShaderModule(glslang_program_SPIRV_get_size(program));
  glslang_program_SPIRV_get(program, outShaderModule.data());

  const char *spirv_messages = glslang_program_SPIRV_get_messages(program);
  if (spirv_messages) {
    LOGI("GLSLang: %s", spirv_messages);
  }

  glslang_program_delete(program);
  glslang_shader_delete(shader);

  return outShaderModule;
}

std::vector<uint32_t> SpvCompiler::compileVertexShader(const char *shaderSource) {
  glslang_initialize_process();
  auto ret = compileShaderInternal(GLSLANG_STAGE_VERTEX, shaderSource);
  glslang_finalize_process();
  return ret;
}

std::vector<uint32_t> SpvCompiler::compileFragmentShader(const char *shaderSource) {
  glslang_initialize_process();
  auto ret = compileShaderInternal(GLSLANG_STAGE_FRAGMENT, shaderSource);
  glslang_finalize_process();
  return ret;
}

}
