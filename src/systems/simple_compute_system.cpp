#include "simple_compute_system.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_c_pipeline.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <memory>
#include <stdexcept>
#include <iostream>

namespace lve
{

  struct SimplePushConstantData
  {
    glm::mat4 modelMatrix{1.f};
    glm::mat4 normalMatrix{1.f};
  };

  SimpleComputeSystem::SimpleComputeSystem(LveDevice &device, VkDescriptorSetLayout textureSetLayout) : lveDevice{device}
  {
    createPipelineLayout(textureSetLayout);
    createPipeLine();
  }
  SimpleComputeSystem::~SimpleComputeSystem()
  {
    vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
  }

  void SimpleComputeSystem::createPipelineLayout(VkDescriptorSetLayout textureSetLayout)
  {

    std::vector<VkDescriptorSetLayout> descriptorSetLayout{textureSetLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayout.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayout.data();

    if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create pipeline layout!");
    }
  }

  void SimpleComputeSystem::createPipeLine()
  {
    assert(pipelineLayout != nullptr &&
           "Cannot create pipeline before pipeline layout");

    ComputePipelineConfigInfo pipelineConfig{};
    LveCPipeline::defaultPipeLineConfigInfo(pipelineConfig);
    pipelineConfig.computePipelineLayout = pipelineLayout;
    lveCPipeline = std::make_unique<LveCPipeline>(
        lveDevice, "shaders/simple_compute_shader.comp.spv", pipelineConfig);
  }

  void SimpleComputeSystem::renderGameObjects(FrameInfo &frameInfo)
  {
    lveCPipeline->bind(frameInfo.commandBuffer);

    // lveGPipeline->bind(frameInfo.commandBuffer);

    // vkCmdBindDescriptorSets(
    //     frameInfo.commandBuffer,
    //     VK_PIPELINE_BIND_POINT_GRAPHICS,
    //     pipelineLayout,
    //     0, 1,
    //     &frameInfo.globalDescriptorSet,
    //     0, nullptr);

    // for (auto &kv : frameInfo.gameObjects)
    // {
    //   auto &obj = kv.second;
    //   if (obj.model == nullptr)
    //     continue;

    //   if (obj.texture != nullptr)
    //   {
    //     vkCmdBindDescriptorSets(
    //         frameInfo.commandBuffer,
    //         VK_PIPELINE_BIND_POINT_GRAPHICS,
    //         pipelineLayout,
    //         1, 1,
    //         &obj.model->textureDescriptorSet,
    //         0, nullptr);
    //   }

    //   SimplePushConstantData push{};
    //   push.modelMatrix = obj.transform.mat4();
    //   push.normalMatrix = obj.transform.normalMatrix();
    //   vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
    //                      VK_SHADER_STAGE_VERTEX_BIT |
    //                          VK_SHADER_STAGE_FRAGMENT_BIT,
    //                      0, sizeof(SimplePushConstantData), &push);
    //   obj.model->bind(frameInfo.commandBuffer);
    //   obj.model->draw(frameInfo.commandBuffer);
    // }
  }

  void SimpleComputeSystem::executeCpS(VkCommandBuffer &commandBuffer, VkDescriptorSet computeDescriptorSets)
  {
    lveCPipeline->bind(commandBuffer);
    
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &computeDescriptorSets, 0, 0);
    std::cout << "Dispatching" << std::endl;
    vkCmdDispatch(commandBuffer, 45, 1, 1);
    std::cout << "cake" << std::endl;
  }
} // namespace lve