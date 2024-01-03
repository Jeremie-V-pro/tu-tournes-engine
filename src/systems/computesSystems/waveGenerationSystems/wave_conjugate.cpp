#include "wave_conjugate.hpp"

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
    unsigned int Size;
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

WaveConjugate::WaveConjugate(LveDevice &device, int height, int width, std::shared_ptr<LveTexture> spectrumTexture,
                             std::shared_ptr<LveTexture> spectrumConjugateTexture)
    : lveDevice{device},
      height{height},
      width{width},
      spectrumTexture{spectrumTexture},
      spectrumConjugateTexture{spectrumConjugateTexture} {
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();

    PipelineCreateInfo pipelineCreateInfo{device,
                                          LvePipeLineType::LvePipeLineTypeCompute,
                                          {waveGenSetLayout->getDescriptorSetLayout()},
                                          {"shaders/wave_texture_spectrumConjugated.comp.spv"},
                                          sizeof(SimplePushConstantData),
                                          LvePipelIneFunctionnality::None,
                                          nullptr};

    pipelineLayout = PipelineBuilder::BuildPipeLineLayout(pipelineCreateInfo);
    lveCPipeline = PipelineBuilder::BuildComputesPipeline(pipelineCreateInfo, pipelineLayout);
}

WaveConjugate::~WaveConjugate() { vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr); }

void WaveConjugate::createDescriptorPool() {
    wavePool = LveDescriptorPool::Builder(lveDevice)
                   .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .build();
}

void WaveConjugate::createDescriptorSetLayout() {
    waveGenSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
                           .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .build();
}

void WaveConjugate::createDescriptorSet() {
    VkDescriptorImageInfo imageSpectrumDescriptorInfo{};
    imageSpectrumDescriptorInfo.imageView = spectrumTexture->getImageView();
    imageSpectrumDescriptorInfo.imageLayout = spectrumTexture->getImageLayout();

    VkDescriptorImageInfo imageSpectrumConjDescriptorInfo1{};
    imageSpectrumConjDescriptorInfo1.imageView = spectrumConjugateTexture->getImageView();
    imageSpectrumConjDescriptorInfo1.imageLayout = spectrumConjugateTexture->getImageLayout();

    LveDescriptorWriter(*waveGenSetLayout, *wavePool)
        .writeImage(0, &imageSpectrumDescriptorInfo)
        .writeImage(1, &imageSpectrumConjDescriptorInfo1)
        .build(waveConjugateDescriptorSets);
}

void WaveConjugate::executePreCpS(FrameInfo frameInfo) {
    VkDescriptorSet descriptorSet[] = {waveConjugateDescriptorSets};

    lveCPipeline->bind(frameInfo.preProcessingCommandBuffer);

    SimplePushConstantData push{};
    push.resolution = glm::vec2(512, 512);
    push.Size = 512;
    vkCmdPushConstants(frameInfo.preProcessingCommandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(SimplePushConstantData), &push);

    vkCmdBindDescriptorSets(frameInfo.preProcessingCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1,
                            descriptorSet, 0, 0);

    vkCmdDispatch(frameInfo.preProcessingCommandBuffer, 512 / 32 + 1, 512 / 32 + 1, 1);
}

}  // namespace lve