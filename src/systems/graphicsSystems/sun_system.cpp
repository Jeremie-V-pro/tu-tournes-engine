#include "sun_system.hpp"

#include <vulkan/vulkan_core.h>

#include <glm/fwd.hpp>
#include <vector>

#include "../pipeline_builder.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_game_object.hpp"
#include "lve_utils.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <map>
#include <stdexcept>

namespace lve {

struct PointLightPushConstants {
    glm::vec4 position{};
    float radius;
};

SunSystem::SunSystem(LveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout,
                     std::shared_ptr<LveGameObject> sun)
    : lveDevice{device}, sun{sun} {
    loadSunTexture();
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
    PipelineCreateInfo pipelineCreateInfo{device,
                                          LvePipeLineType::LvePipeLineTypeRender,
                                          {globalSetLayout, sunSetLayout->getDescriptorSetLayout()},
                                          {"shaders/sun.vert.spv", "shaders/sun.frag.spv"},
                                          sizeof(PointLightPushConstants),
                                          LvePipelIneFunctionnality::Transparancy,
                                          renderPass};

    pipelineLayout = PipelineBuilder::BuildPipeLineLayout(pipelineCreateInfo);
    lveGPipeline = PipelineBuilder::BuildGraphicsPipeline(pipelineCreateInfo, pipelineLayout);
}

SunSystem::~SunSystem() { vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr); }

void SunSystem::createDescriptorPool() {
    sunPool = LveDescriptorPool::Builder(lveDevice)
                  .setMaxSets(1)
                  .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
                  .build();
}

void SunSystem::createDescriptorSetLayout() {
    sunSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
                       .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                       .build();
}

void SunSystem::createDescriptorSet() {
    VkDescriptorImageInfo sunDesc{};
    sunDesc.imageView = sunTexture->getImageView();
    sunDesc.imageLayout = sunTexture->getImageLayout();
    sunDesc.sampler = sunTexture->getSampler();

    LveDescriptorWriter(*sunSetLayout, *sunPool).writeImage(0, &sunDesc).build(sunDescriptorSets);
}

void SunSystem::loadSunTexture() { sunTexture = std::make_shared<LveTexture>(lveDevice, "textures/sun.png", false); }

void SunSystem::render(FrameInfo& frameInfo) {
    lveGPipeline->bind(frameInfo.commandBuffer);
    VkDescriptorSet descriptorSet[] = {frameInfo.globalDescriptorSet, sunDescriptorSets};
    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 2,
                            descriptorSet, 0, nullptr);

    PointLightPushConstants push{};
    push.position = glm::vec4(sun->transform.translation, 1.f);
    push.radius = sun->transform.scale.x;
    vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PointLightPushConstants),
                       &push);

    vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
}

}  // namespace lve