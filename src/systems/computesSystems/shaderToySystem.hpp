#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

#include "../lve_Ipost_processing.hpp"
#include "lve_c_pipeline.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
namespace lve {
class ShaderToySystem : public LveIPostProcessing {
   public:
    ShaderToySystem(LveDevice &device, VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout textureSetLayout,
                    VkDescriptorSetLayout depthSetLayout);
    ~ShaderToySystem();

    void executePostCpS(FrameInfo FrameInfo, VkDescriptorSet computeDescriptorSets, VkDescriptorSet depthDescriptorSets,
                        VkExtent2D extent) override;

   private:
    void createdescriptorSet();

    LveDevice &lveDevice;
    std::unique_ptr<LveCPipeline> lveCPipeline;
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    VkPipelineLayout pipelineLayout;
};
}  // namespace lve