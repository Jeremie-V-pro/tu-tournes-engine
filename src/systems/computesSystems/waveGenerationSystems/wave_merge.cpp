#include "wave_merge.hpp"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <glm/common.hpp>
#include <vector>

#include "../../pipeline_builder.hpp"
#include "lve_c_pipeline.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_swap_chain.hpp"
#include "lve_texture.hpp"
#include "lve_utils.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace lve {

struct SimplePushConstantData {
    glm::vec2 resolution;
    float Lambda;
    float DeltaTime;
};

struct SpectrumParam {
    float scale;
    float angle;
    float spreadBlend;
    float swell;
    float alpha;
    float peakOmega;
    float gamma;
    float shortWavesFade;
};

struct waveGenData {
    SpectrumParam spectrums[2];
    float LengthScale;
    float CutoffLow;
    float CutoffHigh;
    uint Size;
};

WaveMerge::WaveMerge(LveDevice &device, int height, int width, std::vector<std::shared_ptr<LveTexture>> Dx_Dz,
                     std::vector<std::shared_ptr<LveTexture>> Dy_Dxz, std::vector<std::shared_ptr<LveTexture>> Dyx_Dyz,
                     std::vector<std::shared_ptr<LveTexture>> Dxx_Dzz,
                     std::vector<std::shared_ptr<LveTexture>> Displacement,
                     std::vector<std::shared_ptr<LveTexture>> Derivatives,
                     std::vector<std::shared_ptr<LveTexture>> Turbulence)
    : lveDevice{device},
      height{height},
      width{width},
      Dx_Dz{Dx_Dz},
      Dy_Dxz{Dy_Dxz},
      Dyx_Dyz{Dyx_Dyz},
      Dxx_Dzz{Dxx_Dzz},
      Displacement{Displacement},
      Derivatives{Derivatives},
      Turbulence{Turbulence} {
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();

    PipelineCreateInfo pipelineCreateInfo{device,
                                          LvePipeLineType::LvePipeLineTypeCompute,
                                          {waveGenSetLayout->getDescriptorSetLayout()},
                                          {"shaders/wave_texture_merge.comp.spv"},
                                          sizeof(SimplePushConstantData),
                                          LvePipelIneFunctionnality::None,
                                          nullptr};

    pipelineLayout = PipelineBuilder::BuildPipeLineLayout(pipelineCreateInfo);
    lveCPipeline = PipelineBuilder::BuildComputesPipeline(pipelineCreateInfo, pipelineLayout);
}

WaveMerge::~WaveMerge() { vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr); }

void WaveMerge::createDescriptorPool() {
    wavePool = LveDescriptorPool::Builder(lveDevice)
                   .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .build();
}

void WaveMerge::createDescriptorSetLayout() {
    waveGenSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
                           .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .addBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .addBinding(6, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .build();
}

void WaveMerge::createDescriptorSet() {
    waveConjugateDescriptorSets.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo Dx_DzDesc{};
        Dx_DzDesc.imageView = Dx_Dz[i]->getImageView();
        Dx_DzDesc.imageLayout = Dx_Dz[i]->getImageLayout();

        VkDescriptorImageInfo Dy_DxzDesc{};
        Dy_DxzDesc.imageView = Dy_Dxz[i]->getImageView();
        Dy_DxzDesc.imageLayout = Dy_Dxz[i]->getImageLayout();

        VkDescriptorImageInfo Dyx_DyzDesc{};
        Dyx_DyzDesc.imageView = Dyx_Dyz[i]->getImageView();
        Dyx_DyzDesc.imageLayout = Dyx_Dyz[i]->getImageLayout();

        VkDescriptorImageInfo Dxx_DzzDesc{};
        Dxx_DzzDesc.imageView = Dxx_Dzz[i]->getImageView();
        Dxx_DzzDesc.imageLayout = Dxx_Dzz[i]->getImageLayout();

        VkDescriptorImageInfo DisplacementorDesv{};
        DisplacementorDesv.imageView = Displacement[i]->getImageView();
        DisplacementorDesv.imageLayout = Displacement[i]->getImageLayout();

        VkDescriptorImageInfo DerivativesDesv{};
        DerivativesDesv.imageView = Derivatives[i]->getImageView();
        DerivativesDesv.imageLayout = Derivatives[i]->getImageLayout();

        VkDescriptorImageInfo TurbulenceDesc{};
        TurbulenceDesc.imageView = Turbulence[i]->getImageView();
        TurbulenceDesc.imageLayout = Turbulence[i]->getImageLayout();

        LveDescriptorWriter(*waveGenSetLayout, *wavePool)
            .writeImage(0, &Dx_DzDesc)
            .writeImage(1, &Dy_DxzDesc)
            .writeImage(2, &Dyx_DyzDesc)
            .writeImage(3, &Dxx_DzzDesc)
            .writeImage(4, &DisplacementorDesv)
            .writeImage(5, &DerivativesDesv)
            .writeImage(6, &TurbulenceDesc)
            .build(waveConjugateDescriptorSets[i]);
    }
}

void WaveMerge::executePreCpS(FrameInfo frameInfo) {
    VkDescriptorSet descriptorSet[] = {waveConjugateDescriptorSets[frameInfo.frameIndex]};

    lveCPipeline->bind(frameInfo.preProcessingCommandBuffer);

    SimplePushConstantData push{};
    push.resolution = glm::vec2(512, 512);
    push.Lambda = 1;
    push.DeltaTime = frameInfo.frameTime;
    vkCmdPushConstants(frameInfo.preProcessingCommandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(SimplePushConstantData), &push);

    vkCmdBindDescriptorSets(frameInfo.preProcessingCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1,
                            descriptorSet, 0, 0);

    vkCmdDispatch(frameInfo.preProcessingCommandBuffer, 512 / 32 + 1, 512 / 32 + 1, 1);
}

}  // namespace lve