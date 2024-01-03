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
class WaveSpectrum {
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

    WaveSpectrum(LveDevice &device, int height, int width, std::shared_ptr<LveTexture> waveTexture,
                 std::shared_ptr<LveTexture> waveDataTexture, float LengthScale, float CutoffLow, float CutoffHigh);
    ~WaveSpectrum();

    void executePreCpS(FrameInfo FrameInfo);

   private:
    void createWaveDataBuffer();
    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSet();
    void setData(float LengthScale, float CutoffLow, float CutoffHigh);
    void updateWaveParameters();
    float JonswapAlpha(float g, float fetch, float windSpeed);

    void createdescriptorSet();

    LveDevice &lveDevice;

    int width;
    int height;
    std::vector<uint8_t> noiseData;
    std::shared_ptr<LveTexture> noiseTexture;
    std::unique_ptr<LveDescriptorPool> wavePool{};
    std::unique_ptr<LveDescriptorSetLayout> waveGenSetLayout;
    std::unique_ptr<LveBuffer> waveGenDataBuffers;
    std::unique_ptr<LveCPipeline> lveCPipeline;
    std::shared_ptr<LveTexture> waveTexture;
    std::shared_ptr<LveTexture> waveDataTexture;
    VkDescriptorSet waveGenDescriptorSets;

    VkPipelineLayout pipelineLayout;
};
}  // namespace lve