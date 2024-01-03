#include "water_system.hpp"

#include <vulkan/vulkan_core.h>

#include <vector>

#include "../pipeline_builder.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_swap_chain.hpp"
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

WaterSystem::WaterSystem(LveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout,
                         std::vector<std::shared_ptr<LveTexture>> displacementTexture1,
                         std::vector<std::shared_ptr<LveTexture>> derivateTexture1,
                         std::vector<std::shared_ptr<LveTexture>> turbulenceTexture1,
                         std::vector<std::shared_ptr<LveTexture>> displacementTexture2,
                         std::vector<std::shared_ptr<LveTexture>> derivateTexture2,
                         std::vector<std::shared_ptr<LveTexture>> turbulenceTexture2,
                         std::vector<std::shared_ptr<LveTexture>> displacementTexture3,
                         std::vector<std::shared_ptr<LveTexture>> derivateTexture3,
                         std::vector<std::shared_ptr<LveTexture>> turbulenceTexture3)
    : lveDevice{device} {
    createDescriptorSetLayout();
    createDescriptorPool();
    ceateDescriptorSet(displacementTexture1, derivateTexture1, turbulenceTexture1, displacementTexture2,
                       derivateTexture2, turbulenceTexture2, displacementTexture3, derivateTexture3,
                       turbulenceTexture3);
    PipelineCreateInfo pipelineCreateInfo{device,
                                          LvePipeLineType::LvePipeLineTypeRender,
                                          {globalSetLayout, waterTextureSetLayout->getDescriptorSetLayout()},
                                          {"shaders/water.vert.spv", "shaders/water.frag.spv"},
                                          sizeof(SimplePushConstantData),
                                          LvePipelIneFunctionnality::None,
                                          renderPass};

    pipelineLayout = PipelineBuilder::BuildPipeLineLayout(pipelineCreateInfo);
    lveGPipeline = PipelineBuilder::BuildGraphicsPipeline(pipelineCreateInfo, pipelineLayout);
}
WaterSystem::~WaterSystem() { vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr); }

void WaterSystem::createDescriptorSetLayout() {
    waterTextureSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
                                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                                .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                                .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                                .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                                .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                                .addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                                .addBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                                .addBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                                .addBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                                .build();
}

void WaterSystem::createDescriptorPool() {
    TexturePool = LveDescriptorPool::Builder(lveDevice)
                      .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .build();
}

void WaterSystem::ceateDescriptorSet(std::vector<std::shared_ptr<LveTexture>> displacementTexture1,
                                     std::vector<std::shared_ptr<LveTexture>> derivateTexture1,
                                     std::vector<std::shared_ptr<LveTexture>> turbulenceTexture1,
                                     std::vector<std::shared_ptr<LveTexture>> displacementTexture2,
                                     std::vector<std::shared_ptr<LveTexture>> derivateTexture2,
                                     std::vector<std::shared_ptr<LveTexture>> turbulenceTexture2,
                                     std::vector<std::shared_ptr<LveTexture>> displacementTexture3,
                                     std::vector<std::shared_ptr<LveTexture>> derivateTexture3,
                                     std::vector<std::shared_ptr<LveTexture>> turbulenceTexture3) {
    for (int i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        descriptorSets.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);

        VkDescriptorImageInfo displacementDescriptorInfo{};
        displacementDescriptorInfo.imageView = displacementTexture1[i]->getImageView();
        displacementDescriptorInfo.imageLayout = displacementTexture1[i]->getImageLayout();
        displacementDescriptorInfo.sampler = displacementTexture1[i]->getSampler();

        VkDescriptorImageInfo derivateDescriptorInfo{};
        derivateDescriptorInfo.imageView = derivateTexture1[i]->getImageView();
        derivateDescriptorInfo.imageLayout = derivateTexture1[i]->getImageLayout();
        derivateDescriptorInfo.sampler = derivateTexture1[i]->getSampler();

        VkDescriptorImageInfo turbulenceDescriptorInfo{};
        turbulenceDescriptorInfo.imageView = turbulenceTexture1[i]->getImageView();
        turbulenceDescriptorInfo.imageLayout = turbulenceTexture1[i]->getImageLayout();
        turbulenceDescriptorInfo.sampler = turbulenceTexture1[i]->getSampler();

        VkDescriptorImageInfo displacementDescriptorInfo2{};
        displacementDescriptorInfo2.imageView = displacementTexture2[i]->getImageView();
        displacementDescriptorInfo2.imageLayout = displacementTexture2[i]->getImageLayout();
        displacementDescriptorInfo2.sampler = displacementTexture2[i]->getSampler();

        VkDescriptorImageInfo derivateDescriptorInfo2{};
        derivateDescriptorInfo2.imageView = derivateTexture2[i]->getImageView();
        derivateDescriptorInfo2.imageLayout = derivateTexture2[i]->getImageLayout();
        derivateDescriptorInfo2.sampler = derivateTexture2[i]->getSampler();

        VkDescriptorImageInfo turbulenceDescriptorInfo2{};
        turbulenceDescriptorInfo2.imageView = turbulenceTexture2[i]->getImageView();
        turbulenceDescriptorInfo2.imageLayout = turbulenceTexture2[i]->getImageLayout();
        turbulenceDescriptorInfo2.sampler = turbulenceTexture2[i]->getSampler();

        VkDescriptorImageInfo displacementDescriptorInfo3{};
        displacementDescriptorInfo3.imageView = displacementTexture3[i]->getImageView();
        displacementDescriptorInfo3.imageLayout = displacementTexture3[i]->getImageLayout();
        displacementDescriptorInfo3.sampler = displacementTexture3[i]->getSampler();

        VkDescriptorImageInfo derivateDescriptorInfo3{};
        derivateDescriptorInfo3.imageView = derivateTexture3[i]->getImageView();
        derivateDescriptorInfo3.imageLayout = derivateTexture3[i]->getImageLayout();
        derivateDescriptorInfo3.sampler = derivateTexture3[i]->getSampler();

        VkDescriptorImageInfo turbulenceDescriptorInfo3{};
        turbulenceDescriptorInfo3.imageView = turbulenceTexture3[i]->getImageView();
        turbulenceDescriptorInfo3.imageLayout = turbulenceTexture3[i]->getImageLayout();
        turbulenceDescriptorInfo3.sampler = turbulenceTexture3[i]->getSampler();

        LveDescriptorWriter(*waterTextureSetLayout, *TexturePool)
            .writeImage(0, &displacementDescriptorInfo)
            .writeImage(1, &derivateDescriptorInfo)
            .writeImage(2, &turbulenceDescriptorInfo)
            .writeImage(3, &displacementDescriptorInfo2)
            .writeImage(4, &derivateDescriptorInfo2)
            .writeImage(5, &turbulenceDescriptorInfo2)
            .writeImage(6, &displacementDescriptorInfo3)
            .writeImage(7, &derivateDescriptorInfo3)
            .writeImage(8, &turbulenceDescriptorInfo3)
            .build(descriptorSets[i]);
    }
}

void WaterSystem::renderGameObjects(FrameInfo &frameInfo) {
    lveGPipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                            &frameInfo.globalDescriptorSet, 0, nullptr);

    for (auto &kv : frameInfo.gameObjects) {
        auto &obj = kv.second;
        if (obj.water == nullptr) continue;

        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
                                &descriptorSets[frameInfo.frameIndex], 0, nullptr);

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