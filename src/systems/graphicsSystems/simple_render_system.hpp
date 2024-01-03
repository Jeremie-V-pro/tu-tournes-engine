#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

#include "lve_descriptor.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_g_pipeline.hpp"
namespace lve {
class SimpleRenderSystem {
   public:
    SimpleRenderSystem(LveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout,
                       VkDescriptorSetLayout textureSetLayout, std::shared_ptr<LveDescriptorSetLayout> waveLayout,
                       std::vector<VkDescriptorSet> waterSets);
    ~SimpleRenderSystem();

    SimpleRenderSystem(const LveWindow &) = delete;
    SimpleRenderSystem &operator=(const LveWindow &) = delete;

    void renderGameObjects(FrameInfo &frameInfo);

   private:
    std::vector<VkDescriptorSet> waterSets;
    LveDevice &lveDevice;
    std::unique_ptr<LveGPipeline> lveGPipeline;
    VkPipelineLayout pipelineLayout;
};
}  // namespace lve