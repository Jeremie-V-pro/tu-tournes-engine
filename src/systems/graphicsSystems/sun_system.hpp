#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_g_pipeline.hpp"
#include "lve_game_object.hpp"
namespace lve {
class SunSystem {
   public:
    SunSystem(LveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout,
              std::shared_ptr<LveGameObject> sun);
    ~SunSystem();

    SunSystem(const LveWindow &) = delete;
    SunSystem &operator=(const LveWindow &) = delete;

    void update(FrameInfo &frameInfo, GlobalUbo &ubo);
    void render(FrameInfo &frameInfo);

   private:
    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSet();
    void loadSunTexture();
    LveDevice &lveDevice;
    std::unique_ptr<LveGPipeline> lveGPipeline;
    std::shared_ptr<LveGameObject> sun;
    std::unique_ptr<LveDescriptorPool> sunPool{};
    std::unique_ptr<LveDescriptorSetLayout> sunSetLayout;
    std::shared_ptr<LveTexture> sunTexture;
    VkDescriptorSet sunDescriptorSets;
    VkPipelineLayout pipelineLayout;
};
}  // namespace lve