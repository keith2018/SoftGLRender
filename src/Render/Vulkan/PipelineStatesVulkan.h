/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include "Base/UUID.h"
#include "Base/HashUtils.h"
#include "Render/PipelineStates.h"
#include "ShaderProgramVulkan.h"
#include "VKContext.h"

namespace SoftGL {

struct PipelineContainerVK {
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
  VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
};

class PipelineStatesVulkan : public PipelineStates {
 public:
  PipelineStatesVulkan(VKContext &ctx, const RenderStates &states)
      : vkCtx_(ctx), PipelineStates(states) {
    device_ = ctx.device();
  }

  ~PipelineStatesVulkan() override {
    for (auto &kv : pipelineCache_) {
      vkDestroyPipeline(device_, kv.second.graphicsPipeline_, nullptr);
      vkDestroyPipelineLayout(device_, kv.second.pipelineLayout_, nullptr);
    }
    pipelineCache_.clear();
  }

  void create(VkPipelineVertexInputStateCreateInfo &vertexInputInfo,
              ShaderProgramVulkan *program,
              VkRenderPass &renderPass,
              VkSampleCountFlagBits sampleCount) {
    size_t cacheKey = getPipelineCacheKey(program, renderPass, sampleCount);
    auto it = pipelineCache_.find(cacheKey);
    if (it != pipelineCache_.end()) {
      currPipeline_ = it->second;
    } else {
      currPipeline_ = createGraphicsPipeline(vertexInputInfo, program, renderPass, sampleCount);
      pipelineCache_[cacheKey] = currPipeline_;
    }
  }

  inline VkPipeline getGraphicsPipeline() const {
    return currPipeline_.graphicsPipeline_;
  }

  inline VkPipelineLayout getGraphicsPipelineLayout() const {
    return currPipeline_.pipelineLayout_;
  }

 private:
  PipelineContainerVK createGraphicsPipeline(VkPipelineVertexInputStateCreateInfo &vertexInputInfo,
                                             ShaderProgramVulkan *program,
                                             VkRenderPass &renderPass,
                                             VkSampleCountFlagBits sampleCount) {
    PipelineContainerVK ret{};

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK::cvtPrimitiveTopology(renderStates.primitiveType);
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK::cvtPolygonMode(renderStates.polygonMode);  // ignore VkPhysicalDeviceFeatures->fillModeNonSolid
    rasterizer.lineWidth = renderStates.lineWidth;
    rasterizer.cullMode = renderStates.cullFace ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
    multisampling.rasterizationSamples = sampleCount;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = renderStates.depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = renderStates.depthMask ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = VK::cvtDepthFunc(renderStates.depthFunc);
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = renderStates.blend ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.colorBlendOp = VK::cvtBlendFunction(renderStates.blendParams.blendFuncRgb);
    colorBlendAttachment.srcColorBlendFactor = VK::cvtBlendFactor(renderStates.blendParams.blendSrcRgb);
    colorBlendAttachment.dstColorBlendFactor = VK::cvtBlendFactor(renderStates.blendParams.blendDstRgb);
    colorBlendAttachment.alphaBlendOp = VK::cvtBlendFunction(renderStates.blendParams.blendFuncAlpha);
    colorBlendAttachment.srcAlphaBlendFactor = VK::cvtBlendFactor(renderStates.blendParams.blendSrcAlpha);
    colorBlendAttachment.dstAlphaBlendFactor = VK::cvtBlendFactor(renderStates.blendParams.blendDstAlpha);

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();

    auto &descriptorSetLayouts = program->getDescriptorSetLayouts();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    VK_CHECK(vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &ret.pipelineLayout_));

    auto &shaderStages = program->getShaderStages();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = ret.pipelineLayout_;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VK_CHECK(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ret.graphicsPipeline_));

    return ret;
  }

  static size_t getPipelineCacheKey(ShaderProgramVulkan *program,
                                    VkRenderPass &renderPass,
                                    VkSampleCountFlagBits sampleCount) {
    size_t seed = 0;

    HashUtils::hashCombine(seed, (void *) program);
    HashUtils::hashCombine(seed, (void *) renderPass);
    HashUtils::hashCombine(seed, (uint32_t) sampleCount);

    return seed;
  }

 private:
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  PipelineContainerVK currPipeline_{};
  std::unordered_map<size_t, PipelineContainerVK> pipelineCache_;
};

}
