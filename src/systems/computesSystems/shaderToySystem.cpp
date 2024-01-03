#include "shaderToySystem.hpp"

#include <vulkan/vulkan_core.h>

#include <vector>

#include "../pipeline_builder.hpp"
#include "lve_c_pipeline.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_utils.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace lve {

struct SimplePushConstantData {
    glm::vec2 resolution;
};

ShaderToySystem::ShaderToySystem(LveDevice &device, VkDescriptorSetLayout globalSetLayout,
                                 VkDescriptorSetLayout textureSetLayout, VkDescriptorSetLayout depthSetLayout)
    : lveDevice{device} {
    PipelineCreateInfo pipelineCreateInfo{device,
                                          LvePipeLineType::LvePipeLineTypeCompute,
                                          {globalSetLayout, textureSetLayout, depthSetLayout},
                                          {"shaders/clouds.comp.spv"},
                                          sizeof(SimplePushConstantData),
                                          LvePipelIneFunctionnality::None,
                                          nullptr};

    pipelineLayout = PipelineBuilder::BuildPipeLineLayout(pipelineCreateInfo);
    lveCPipeline = PipelineBuilder::BuildComputesPipeline(pipelineCreateInfo, pipelineLayout);
}
ShaderToySystem::~ShaderToySystem() { vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr); }

void ShaderToySystem::executePostCpS(FrameInfo frameInfo, VkDescriptorSet computeDescriptorSets,
                                     VkDescriptorSet depthDescriptorSets, VkExtent2D windowExtent) {
    VkDescriptorSet descriptorSet[] = {frameInfo.globalDescriptorSet, computeDescriptorSets, depthDescriptorSets};

    lveCPipeline->bind(frameInfo.postProcessingCommandBuffer);

    SimplePushConstantData push{};
    push.resolution = glm::vec2(windowExtent.width, windowExtent.height);
    vkCmdPushConstants(frameInfo.postProcessingCommandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(SimplePushConstantData), &push);

    vkCmdBindDescriptorSets(frameInfo.postProcessingCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 3,
                            descriptorSet, 0, 0);

    vkCmdDispatch(frameInfo.postProcessingCommandBuffer, windowExtent.width / 32 + 1, windowExtent.height / 32 + 1, 1);
}
}  // namespace lve