#include "simple_compute_systemB.hpp"
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


  SimpleComputeSystemB::SimpleComputeSystemB(LveDevice &device, VkDescriptorSetLayout textureSetLayout) : lveDevice{device}
  {
    createPipelineLayout(textureSetLayout);
    createPipeLine();
  }
  SimpleComputeSystemB::~SimpleComputeSystemB()
  {
    vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
  }

  void SimpleComputeSystemB::createPipelineLayout(VkDescriptorSetLayout textureSetLayout)
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

  void SimpleComputeSystemB::createPipeLine()
  {
    assert(pipelineLayout != nullptr &&
           "Cannot create pipeline before pipeline layout");

    ComputePipelineConfigInfo pipelineConfig{};
    LveCPipeline::defaultPipeLineConfigInfo(pipelineConfig);
    pipelineConfig.computePipelineLayout = pipelineLayout;
    lveCPipeline = std::make_unique<LveCPipeline>(
        lveDevice, "shaders/simple_compute_shaderB.comp.spv", pipelineConfig);
  }

  void SimpleComputeSystemB::executeCpS(FrameInfo frameInfo, VkDescriptorSet computeDescriptorSets, VkExtent2D windowExtent)
  {
    lveCPipeline->bind(frameInfo.computeCommandBuffer);
    
    vkCmdBindDescriptorSets(frameInfo.computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &computeDescriptorSets, 0, 0);

    vkCmdDispatch(frameInfo.computeCommandBuffer, windowExtent.width, windowExtent.height, 1);

  }
} // namespace lve