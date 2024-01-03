#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_g_pipeline.hpp"
namespace lve {
class WaterSystem {
   public:
    WaterSystem(LveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout,
                std::vector<std::shared_ptr<LveTexture>> displacementTexture1,
                std::vector<std::shared_ptr<LveTexture>> derivateTexture1,
                std::vector<std::shared_ptr<LveTexture>> turbulenceTexture1,
                std::vector<std::shared_ptr<LveTexture>> displacementTexture2,
                std::vector<std::shared_ptr<LveTexture>> derivateTexture2,
                std::vector<std::shared_ptr<LveTexture>> turbulenceTexture2,
                std::vector<std::shared_ptr<LveTexture>> displacementTexture3,
                std::vector<std::shared_ptr<LveTexture>> derivateTexture3,
                std::vector<std::shared_ptr<LveTexture>> turbulenceTexture3);
    ~WaterSystem();

    WaterSystem(const LveWindow &) = delete;
    WaterSystem &operator=(const LveWindow &) = delete;

    void renderGameObjects(FrameInfo &frameInfo);

    std::shared_ptr<LveDescriptorSetLayout> getWaterTextureSetLayout() { return waterTextureSetLayout; }
    std::vector<VkDescriptorSet> getDescriptorSets() { return descriptorSets; }

   private:
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void ceateDescriptorSet(std::vector<std::shared_ptr<LveTexture>> displacementTexture1,
                            std::vector<std::shared_ptr<LveTexture>> derivateTexture1,
                            std::vector<std::shared_ptr<LveTexture>> turbulenceTexture1,
                            std::vector<std::shared_ptr<LveTexture>> displacementTexture2,
                            std::vector<std::shared_ptr<LveTexture>> derivateTexture2,
                            std::vector<std::shared_ptr<LveTexture>> turbulenceTexture2,
                            std::vector<std::shared_ptr<LveTexture>> displacementTexture3,
                            std::vector<std::shared_ptr<LveTexture>> derivateTexture3,
                            std::vector<std::shared_ptr<LveTexture>> turbulenceTexture3);

    std::shared_ptr<LveDescriptorSetLayout> waterTextureSetLayout;
    std::unique_ptr<LveDescriptorPool> TexturePool{};
    std::vector<VkDescriptorSet> descriptorSets;

    LveDevice &lveDevice;
    std::unique_ptr<LveGPipeline> lveGPipeline;
    VkPipelineLayout pipelineLayout;
};
}  // namespace lve