#include "wave_Scale.hpp"

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

struct waveGenData {
    SpectrumParam spectrums[2];
    float LengthScale;
    float CutoffLow;
    float CutoffHigh;
    uint Size;
};

WaveScale::WaveScale(LveDevice &device, int height, int width, std::vector<std::shared_ptr<LveTexture>> buffer0)
    : lveDevice{device}, height{height}, width{width}, buffer0{buffer0} {
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();

    PipelineCreateInfo pipelineCreateInfo{device,
                                          LvePipeLineType::LvePipeLineTypeCompute,
                                          {waveGenSetLayout->getDescriptorSetLayout()},
                                          {"shaders/wave_textureScale.comp.spv"},
                                          sizeof(SimplePushConstantData),
                                          LvePipelIneFunctionnality::None,
                                          nullptr};

    pipelineLayout = PipelineBuilder::BuildPipeLineLayout(pipelineCreateInfo);
    lveCPipeline = PipelineBuilder::BuildComputesPipeline(pipelineCreateInfo, pipelineLayout);
}

WaveScale::~WaveScale() { vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr); }

void WaveScale::createDescriptorPool() {
    wavePool = LveDescriptorPool::Builder(lveDevice)
                   .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .build();
}

void WaveScale::createDescriptorSetLayout() {
    waveGenSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
                           .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .build();
}

void WaveScale::createDescriptorSet() {
    waveConjugateDescriptorSets.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo bufferDescriptorInfo0{};
        bufferDescriptorInfo0.imageView = buffer0[i]->getImageView();
        bufferDescriptorInfo0.imageLayout = buffer0[i]->getImageLayout();

        LveDescriptorWriter(*waveGenSetLayout, *wavePool)
            .writeImage(0, &bufferDescriptorInfo0)
            .build(waveConjugateDescriptorSets[i]);
    }
}

void WaveScale::executePreCpS(FrameInfo frameInfo) {
    VkDescriptorSet descriptorSet[] = {waveConjugateDescriptorSets[frameInfo.frameIndex]};

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