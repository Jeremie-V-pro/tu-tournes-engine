#pragma once

#include <vulkan/vulkan_core.h>

#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "lve_c_pipeline.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_texture.hpp"

namespace lve {
class WaveTimeUpdate {
   public:
    struct WaveCreationStriuct {
        // waveGeneration
        LveTexture waveTexture;
        LveTexture waveTextureConj;
        LveTexture waveData;
        // For IFFT
        LveTexture Dx_Dz;
        LveTexture Dy_Dxz;
        LveTexture Dyx_Dyz;
        LveTexture Dxx_Dzz;
        LveTexture precomputedData;
        LveTexture buffer[4];
        // Result
        LveTexture Displacement;
        LveTexture Derivatives;
        LveTexture Turbulence;
    };

    WaveTimeUpdate(LveDevice &device, int height, int width, std::vector<std::shared_ptr<LveTexture>> Dx_Dz,
                   std::vector<std::shared_ptr<LveTexture>> Dy_Dxz, std::vector<std::shared_ptr<LveTexture>> Dyx_Dyz,
                   std::vector<std::shared_ptr<LveTexture>> Dxx_Dzz, std::shared_ptr<LveTexture> spectrumConjugate,
                   std::shared_ptr<LveTexture> WavesData);
    ~WaveTimeUpdate();

    void executePreCpS(FrameInfo FrameInfo);

   private:
    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSet();

    LveDevice &lveDevice;

    int width;
    int height;
    float time = 0.0f;
    std::vector<std::shared_ptr<LveTexture>> Dx_Dz;
    std::vector<std::shared_ptr<LveTexture>> Dy_Dxz;
    std::vector<std::shared_ptr<LveTexture>> Dyx_Dyz;
    std::vector<std::shared_ptr<LveTexture>> Dxx_Dzz;
    std::shared_ptr<LveTexture> spectrum;
    std::shared_ptr<LveTexture> WavesData;

    std::vector<VkDescriptorSet> waveConjugateDescriptorSets;
    std::unique_ptr<LveDescriptorSetLayout> waveGenSetLayout;

    std::unique_ptr<LveDescriptorPool> wavePool{};
    std::unique_ptr<LveCPipeline> lveCPipeline;
    VkPipelineLayout pipelineLayout;
};
}  // namespace lve