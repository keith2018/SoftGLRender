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

struct UniformInfoVulkan {
  ShaderUniformType type;
  uint32_t binding;
  VkShaderStageFlags stageFlags;
};

class ShaderProgramVulkan : public ShaderProgram {
 public:
  explicit ShaderProgramVulkan(VKContext &ctx) : vkCtx_(ctx) {
    device_ = ctx.device();
    glslHeader_ = "#version 450\n";
  }

  ~ShaderProgramVulkan() {
    vkDestroyShaderModule(device_, vertexShader_, nullptr);
    vkDestroyShaderModule(device_, fragmentShader_, nullptr);

    vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
    for (auto &setLayout : descriptorSetLayouts_) {
      vkDestroyDescriptorSetLayout(device_, setLayout, nullptr);
    }
  }

  int getId() const override {
    return uuid_.get();
  }

  void addDefine(const std::string &def) override {
    if (def.empty()) {
      return;
    }
    glslDefines_ += ("#define " + def + " \n");
  }

  bool compileAndLinkGLSL(const std::string &vsSource, const std::string &fsSource) {
    std::string vsStr = glslHeader_ + glslDefines_ + vsSource;
    std::string fsStr = glslHeader_ + glslDefines_ + fsSource;

    auto vsData = SpvCompiler::compileVertexShader(vsStr.c_str());
    if (vsData.spvCodes.empty()) {
      LOGE("load vertex shader file failed");
      return false;
    }

    auto fsData = SpvCompiler::compileFragmentShader(fsStr.c_str());
    if (fsData.spvCodes.empty()) {
      LOGE("load fragment shader file failed");
      return false;
    }

    createShaderModule(vertexShader_, vsData.spvCodes.data(), vsData.spvCodes.size() * sizeof(uint32_t));
    createShaderModule(fragmentShader_, fsData.spvCodes.data(), fsData.spvCodes.size() * sizeof(uint32_t));
    createShaderStages();

    createDescriptorSetLayouts(vsData.uniformsDesc, fsData.uniformsDesc);
    createDescriptorPool();
    createDescriptorSets();

    return true;
  }

  inline std::vector<VkPipelineShaderStageCreateInfo> &getShaderStages() {
    return shaderStages_;
  }

  inline std::vector<VkDescriptorSetLayout> &getDescriptorSetLayouts() {
    return descriptorSetLayouts_;
  }

  inline std::vector<VkDescriptorSet> &getVkDescriptorSet() {
    return descriptorSets_;
  }

  inline int getUniformLocation(const std::string &name) {
    auto it = uniformsInfo_.find(name);
    if (it != uniformsInfo_.end()) {
      return (int) it->second.binding;
    }
    return -1;
  }

  void bindUniformBuffer(VkDescriptorBufferInfo &info, uint32_t binding) {
    VkWriteDescriptorSet writeDesc{};
    writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDesc.dstSet = descriptorSets_[0];   // all uniform bind to set 0
    writeDesc.dstBinding = binding;
    writeDesc.dstArrayElement = 0;
    writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDesc.descriptorCount = 1;
    writeDesc.pBufferInfo = &info;
    writeDescriptorSets_.push_back(writeDesc);
  }

  bool bindUniformsBegin(size_t uniformCnt = 0) {
    if (!writeDescriptorSets_.empty()) {
      return false;
    }
    if (uniformCnt > 0) {
      writeDescriptorSets_.reserve(uniformCnt);
    }
    return true;
  }

  void bindUniformsEnd() {
    if (writeDescriptorSets_.empty()) {
      return;
    }
    vkUpdateDescriptorSets(device_, writeDescriptorSets_.size(), writeDescriptorSets_.data(), 0, nullptr);
  }

 private:
  void createDescriptorSetLayouts(std::unordered_map<std::string, ShaderUniformDesc> &uniformsDescVertex,
                                  std::unordered_map<std::string, ShaderUniformDesc> &uniformsDescFragment) {
    for (auto &kv : uniformsDescVertex) {
      uniformsInfo_[kv.first].type = kv.second.type;
      uniformsInfo_[kv.first].stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    for (auto &kv : uniformsDescFragment) {
      uniformsInfo_[kv.first].type = kv.second.type;
      uniformsInfo_[kv.first].stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    uint32_t binding = 0;
    for (auto &kv : uniformsInfo_) {
      kv.second.binding = binding++;

      VkDescriptorSetLayoutBinding layoutBinding{};
      layoutBinding.binding = kv.second.binding;
      layoutBinding.descriptorCount = 1;
      layoutBinding.descriptorType = getDescriptorType(kv.second.type);
      layoutBinding.pImmutableSamplers = nullptr;
      layoutBinding.stageFlags = kv.second.stageFlags;
      bindings.push_back(layoutBinding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    descriptorSetLayouts_.resize(1);  // only one set, all uniform bind to set 0
    VK_CHECK(vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayouts_[0]));
  }

  static VkDescriptorType getDescriptorType(ShaderUniformType type) {
    switch (type) {
      case UniformType_Block:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      case UniformType_Sampler:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      default:
        break;
    }
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
  }

  void createDescriptorPool() {
    uint32_t uniformBlockCnt = 0;
    uint32_t uniformSamplerCnt = 0;
    for (auto &kv : uniformsInfo_) {
      switch (kv.second.type) {
        case UniformType_Block:
          uniformBlockCnt++;
          break;
        case UniformType_Sampler:
          uniformSamplerCnt++;
          break;
        default:
          break;
      }
    }

    std::vector<VkDescriptorPoolSize> poolSizes;
    if (uniformBlockCnt > 0) {
      poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBlockCnt});
    }
    if (uniformSamplerCnt > 0) {
      poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uniformSamplerCnt});
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = descriptorSetLayouts_.size();

    VK_CHECK(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_));
  }

  void createDescriptorSets() {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = descriptorSetLayouts_.size();
    allocInfo.pSetLayouts = descriptorSetLayouts_.data();

    descriptorSets_.resize(descriptorSetLayouts_.size());
    VK_CHECK(vkAllocateDescriptorSets(device_, &allocInfo, descriptorSets_.data()));
  }

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
  std::vector<VkDescriptorSetLayout> descriptorSetLayouts_;
  VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets_;
  std::vector<VkWriteDescriptorSet> writeDescriptorSets_;

  std::string glslHeader_;
  std::string glslDefines_;
  std::unordered_map<std::string, UniformInfoVulkan> uniformsInfo_;
};

}
