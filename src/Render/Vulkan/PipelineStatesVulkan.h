/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/UUID.h"
#include "Render/PipelineStates.h"
#include "ShaderProgramVulkan.h"
#include "VKContext.h"

namespace SoftGL {

class PipelineStatesVulkan : public PipelineStates {
 public:
  PipelineStatesVulkan(VKContext &ctx, const RenderStates &states)
      : vkCtx_(ctx), PipelineStates(states) {
    device_ = ctx.device();
  }

  ~PipelineStatesVulkan() override {
    vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
  }

  void create(VkPipelineVertexInputStateCreateInfo &vertexInputInfo,
              ShaderProgramVulkan *program,
              VkRenderPass renderPass) {
    createGraphicsPipeline(vertexInputInfo, program, renderPass);
  }

  inline VkPipeline getGraphicsPipeline() {
    return graphicsPipeline_;
  }

  inline VkPipelineLayout getGraphicsPipelineLayout() {
    return pipelineLayout_;
  }

 private:
  void createGraphicsPipeline(VkPipelineVertexInputStateCreateInfo &vertexInputInfo,
                              ShaderProgramVulkan *program,
                              VkRenderPass renderPass) {
    if (graphicsPipeline_ != VK_NULL_HANDLE) {
      return;
    }

    if (!program || !renderPass) {
      return;
    }

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK::cvtPrimitiveTopology(renderStates.primitiveType);
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 0;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK::cvtPolygonMode(renderStates.polygonMode);
    rasterizer.lineWidth = renderStates.lineWidth;
    rasterizer.cullMode = renderStates.cullFace ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

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
        VK_DYNAMIC_STATE_VIEWPORT
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    auto &descriptorSetLayouts = program->getDescriptorSetLayouts();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    VK_CHECK(vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_));

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
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VK_CHECK(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_));
  }

 private:
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
  VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
};

}
