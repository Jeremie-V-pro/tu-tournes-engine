#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "lve_c_pipeline.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_texture.hpp"
namespace lve {
class WaveConjugate {
   public:
    WaveConjugate(LveDevice &device, int height, int width, std::shared_ptr<LveTexture> spectrumTexture,
                  std::shared_ptr<LveTexture> spectrumConjugateTexture);
    ~WaveConjugate();

    void executePreCpS(FrameInfo FrameInfo);

   private:
    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSet();

    LveDevice &lveDevice;

    int width;
    int height;
    std::shared_ptr<LveTexture> spectrumTexture;
    std::shared_ptr<LveTexture> spectrumConjugateTexture;
    VkDescriptorSet waveConjugateDescriptorSets;
    std::unique_ptr<LveDescriptorSetLayout> waveGenSetLayout;

    std::unique_ptr<LveDescriptorPool> wavePool{};
    std::unique_ptr<LveCPipeline> lveCPipeline;
    VkPipelineLayout pipelineLayout;
};
}  // namespace lve