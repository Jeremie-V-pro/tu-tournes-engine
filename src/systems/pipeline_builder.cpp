#include "pipeline_builder.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

#include "../lve_c_pipeline.hpp"
#include "../lve_g_pipeline.hpp"
#include "../lve_utils.hpp"

namespace lve {

VkPipelineLayout PipelineBuilder::BuildPipeLineLayout(PipelineCreateInfo &pipelineCreateInfo) {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    VkPushConstantRange pushConstantRange{};
    if (pipelineCreateInfo.pushConstantRangeSize) {
        switch (pipelineCreateInfo.type) {
            case LvePipeLineType::LvePipeLineTypeRender:
                pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case LvePipeLineType::LvePipeLineTypeCompute:
                pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default:
                throw std::runtime_error("Forget to set pipeline type");
                break;
        }
        pushConstantRange.offset = 0;
        pushConstantRange.size = pipelineCreateInfo.pushConstantRangeSize;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    }

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(pipelineCreateInfo.SetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = pipelineCreateInfo.SetLayouts.data();

    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(pipelineCreateInfo.device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
    return pipelineLayout;
}

std::unique_ptr<LveGPipeline> PipelineBuilder::BuildGraphicsPipeline(PipelineCreateInfo &pipelineCreateInfo,
                                                                     VkPipelineLayout pipelineLayout) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    LveGPipeline::defaultPipeLineConfigInfo(pipelineConfig);
    if (pipelineCreateInfo.functionnality & LvePipelIneFunctionnality::Transparancy) {
        LveGPipeline::enableAlphaBlending(pipelineConfig);
        pipelineConfig.attributeDescriptions.clear();
        pipelineConfig.bindingDescriptions.clear();
    }

    pipelineConfig.renderPass = pipelineCreateInfo.renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;

    return std::make_unique<LveGPipeline>(pipelineCreateInfo.device, pipelineCreateInfo.shaderPaths[0],
                                          pipelineCreateInfo.shaderPaths[1], pipelineConfig);
}

std::unique_ptr<LveCPipeline> PipelineBuilder::BuildComputesPipeline(PipelineCreateInfo &pipelineCreateInfo,
                                                                     VkPipelineLayout pipelineLayout) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    ComputePipelineConfigInfo pipelineConfig{};
    LveCPipeline::defaultPipeLineConfigInfo(pipelineConfig);
    pipelineConfig.computePipelineLayout = pipelineLayout;
    return std::make_unique<LveCPipeline>(pipelineCreateInfo.device, pipelineCreateInfo.shaderPaths[0], pipelineConfig);
}

}  // namespace lve