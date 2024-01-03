#include "simple_render_system.hpp"

#include <vulkan/vulkan_core.h>

#include <vector>

#include "../pipeline_builder.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_utils.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace lve {

struct SimplePushConstantData {
    glm::mat4 modelMatrix{1.f};
    glm::mat4 normalMatrix{1.f};
};

SimpleRenderSystem::SimpleRenderSystem(LveDevice &device, VkRenderPass renderPass,
                                       VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout textureSetLayout,
                                       std::shared_ptr<LveDescriptorSetLayout> waveLayout,
                                       std::vector<VkDescriptorSet> waterSets)
    : lveDevice{device}, waterSets{waterSets} {
    PipelineCreateInfo pipelineCreateInfo{device,
                                          LvePipeLineType::LvePipeLineTypeRender,
                                          {globalSetLayout, textureSetLayout, waveLayout->getDescriptorSetLayout()},
                                          {"shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv"},
                                          sizeof(SimplePushConstantData),
                                          LvePipelIneFunctionnality::None,
                                          renderPass};

    pipelineLayout = PipelineBuilder::BuildPipeLineLayout(pipelineCreateInfo);
    lveGPipeline = PipelineBuilder::BuildGraphicsPipeline(pipelineCreateInfo, pipelineLayout);
}
SimpleRenderSystem::~SimpleRenderSystem() { vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr); }

void SimpleRenderSystem::renderGameObjects(FrameInfo &frameInfo) {
    lveGPipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                            &frameInfo.globalDescriptorSet, 0, nullptr);

    for (auto &kv : frameInfo.gameObjects) {
        auto &obj = kv.second;
        if (obj.model == nullptr || obj.water != nullptr) continue;

        if (obj.texture != nullptr) {
            vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
                                    &obj.model->textureDescriptorSet, 0, nullptr);
        }
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1,
                                &waterSets[frameInfo.frameIndex], 0, nullptr);
        SimplePushConstantData push{};
        push.modelMatrix = obj.transform.mat4();
        push.normalMatrix = obj.transform.normalMatrix();
        vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData),
                           &push);
        obj.model->bind(frameInfo.commandBuffer);
        obj.model->draw(frameInfo.commandBuffer);
    }
}

}  // namespace lve