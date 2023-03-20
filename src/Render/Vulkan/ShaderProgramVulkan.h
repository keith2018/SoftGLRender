/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/UUID.h"
#include "Base/FileUtils.h"
#include "Render/ShaderProgram.h"
#include "SPIRV/SpvCompiler.h"
#include "VulkanUtils.h"

namespace SoftGL {

class ShaderProgramVulkan : public ShaderProgram {
 public:
  explicit ShaderProgramVulkan(VKContext &ctx) : vkCtx_(ctx) {
    device_ = ctx.device();
  }

  ~ShaderProgramVulkan() {
    vkDestroyShaderModule(device_, vertexShader_, nullptr);
    vkDestroyShaderModule(device_, fragmentShader_, nullptr);
  }

  int getId() const override {
    return uuid_.get();
  }

  void addDefine(const std::string &def) override {
  }

  bool compileAndLinkGLSL(const std::string &vsSource, const std::string &fsSource) {
    auto vsData = SpvCompiler::compileVertexShader(vsSource.c_str());
    if (vsData.empty()) {
      LOGE("load vertex shader file failed");
      return false;
    }

    auto fsData = SpvCompiler::compileFragmentShader(fsSource.c_str());
    if (fsData.empty()) {
      LOGE("load fragment shader file failed");
      return false;
    }

    createShaderModule(vertexShader_, vsData.data(), vsData.size() * sizeof(uint32_t));
    createShaderModule(fragmentShader_, fsData.data(), fsData.size() * sizeof(uint32_t));

    createShaderStages();
    return true;
  }

  bool compileAndLinkSpv(const std::string &vsPath, const std::string &fsPath) {
    auto vsData = FileUtils::readBytes(vsPath);
    if (vsData.empty()) {
      LOGE("load vertex shader file failed");
      return false;
    }

    auto fsData = FileUtils::readBytes(fsPath);
    if (fsData.empty()) {
      LOGE("load fragment shader file failed");
      return false;
    }

    createShaderModule(vertexShader_, reinterpret_cast<const uint32_t *>(vsData.data()), vsData.size());
    createShaderModule(fragmentShader_, reinterpret_cast<const uint32_t *>(fsData.data()), fsData.size());

    createShaderStages();
    return true;
  }

  inline std::vector<VkPipelineShaderStageCreateInfo> &getShaderStages() {
    return shaderStages_;
  }

 private:
  void createShaderStages() {
    shaderStages_.resize(2);
    auto &vertShaderStageInfo = shaderStages_[0];
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertexShader_;
    vertShaderStageInfo.pName = "main";

    auto &fragShaderStageInfo = shaderStages_[1];
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragmentShader_;
    fragShaderStageInfo.pName = "main";
  }

  void createShaderModule(VkShaderModule &shaderModule, const uint32_t *spvCode, size_t codeSize) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeSize;
    createInfo.pCode = spvCode;

    VK_CHECK(vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule));
  }

 private:
  UUID<ShaderProgramVulkan> uuid_;
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  VkShaderModule vertexShader_ = VK_NULL_HANDLE;
  VkShaderModule fragmentShader_ = VK_NULL_HANDLE;
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages_;
};

}
