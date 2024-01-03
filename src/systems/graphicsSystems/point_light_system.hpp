#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_g_pipeline.hpp"
namespace lve {
class PointLightSystem {
   public:
    PointLightSystem(LveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~PointLightSystem();

    PointLightSystem(const LveWindow &) = delete;
    PointLightSystem &operator=(const LveWindow &) = delete;

    void update(FrameInfo &frameInfo, GlobalUbo &ubo);
    void render(FrameInfo &frameInfo);

   private:
    float time;
    LveDevice &lveDevice;
    std::unique_ptr<LveGPipeline> lveGPipeline;
    VkPipelineLayout pipelineLayout;
};
}  // namespace lve