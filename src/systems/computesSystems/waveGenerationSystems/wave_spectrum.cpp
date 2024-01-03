#include "wave_spectrum.hpp"

#include <vulkan/vulkan_core.h>

#include <cmath>
#include <cstdint>
#include <glm/common.hpp>
#include <vector>

#include "../../pipeline_builder.hpp"
#include "lve_c_pipeline.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_texture.hpp"
#include "lve_utils.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <math.h>

#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace lve {

struct SimplePushConstantData {
    glm::vec2 resolution;
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

std::vector<float> loadNoise() {
    const std::string filePath = "textures/noise.csv";

    // Open the file
    std::ifstream inputFile(ENGINE_DIR + filePath);
    // Check if the file is open
    if (!inputFile.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    // Vector to store the float values
    std::vector<float> dataVector;

    // Read each line from the file
    std::string line;
    while (std::getline(inputFile, line)) {
        // Use a stringstream to extract float values between quotes
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, ',')) {
            // Convert the string to float and push it to the vector
            float value = std::stof(token);
            dataVector.push_back(value);
        }
    }

    // Close the file
    inputFile.close();
    return dataVector;
}
WaveSpectrum::WaveSpectrum(LveDevice &device, int height, int width, std::shared_ptr<LveTexture> waveTexture,
                           std::shared_ptr<LveTexture> waveDataTexture, float LengthScale, float CutoffLow,
                           float CutoffHigh)
    : lveDevice{device}, height{height}, width{width}, waveTexture{waveTexture}, waveDataTexture{waveDataTexture} {
    noiseTexture = std::make_shared<LveTexture>(lveDevice, 512, 512, loadNoise().data(), 2, VK_FORMAT_R32G32_SFLOAT);
    createWaveDataBuffer();
    setData(LengthScale, CutoffLow, CutoffHigh);
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();

    PipelineCreateInfo pipelineCreateInfo{device,
                                          LvePipeLineType::LvePipeLineTypeCompute,
                                          {waveGenSetLayout->getDescriptorSetLayout()},
                                          {"shaders/wave_texture_spectrum.comp.spv"},
                                          sizeof(SimplePushConstantData),
                                          LvePipelIneFunctionnality::None,
                                          nullptr};

    pipelineLayout = PipelineBuilder::BuildPipeLineLayout(pipelineCreateInfo);
    lveCPipeline = PipelineBuilder::BuildComputesPipeline(pipelineCreateInfo, pipelineLayout);
}

WaveSpectrum::~WaveSpectrum() { vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr); }

void WaveSpectrum::createWaveDataBuffer() {
    waveGenDataBuffers = std::make_unique<LveBuffer>(
        lveDevice, sizeof(waveGenData), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    waveGenDataBuffers->map();
}

void WaveSpectrum::createDescriptorPool() {
    wavePool = LveDescriptorPool::Builder(lveDevice)
                   .setMaxSets(1)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1)
                   .build();
}

void WaveSpectrum::createDescriptorSetLayout() {
    waveGenSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
                           .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                           .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .build();
}

void WaveSpectrum::createDescriptorSet() {
    VkDescriptorImageInfo imageNoiseDescriptorInfo{};
    imageNoiseDescriptorInfo.imageView = noiseTexture->getImageView();
    imageNoiseDescriptorInfo.imageLayout = noiseTexture->getImageLayout();

    VkDescriptorImageInfo imageWaveDescriptorInfo{};
    imageWaveDescriptorInfo.imageView = waveTexture->getImageView();
    imageWaveDescriptorInfo.imageLayout = waveTexture->getImageLayout();

    VkDescriptorImageInfo imageWaveDataDescriptorInfo{};
    imageWaveDataDescriptorInfo.imageView = waveDataTexture->getImageView();
    imageWaveDataDescriptorInfo.imageLayout = waveDataTexture->getImageLayout();

    auto bufferInfo = waveGenDataBuffers->descriptorInfo();
    LveDescriptorWriter(*waveGenSetLayout, *wavePool)
        .writeBuffer(0, &bufferInfo)
        .writeImage(1, &imageNoiseDescriptorInfo)
        .writeImage(2, &imageWaveDescriptorInfo)
        .writeImage(3, &imageWaveDataDescriptorInfo)
        .build(waveGenDescriptorSets);
}

float WaveSpectrum::JonswapAlpha(float g, float fetch, float windSpeed) {
    return 0.076f * pow(g * fetch / windSpeed / windSpeed, -0.22f);
}

float JonswapPeakFrequency(float g, float fetch, float windSpeed) {
    return 22 * pow(windSpeed * fetch / g / g, -0.33f);
}

void WaveSpectrum::setData(float LengthScale, float CutoffLow, float CutoffHigh) {
    SpectrumParam spectrum1;
    SpectrumParam spectrum2;

    spectrum1.scale = 0.5;
    spectrum1.angle = -29.81 / 180.0 * M_PI;
    spectrum1.spreadBlend = 1;
    spectrum1.swell = glm::clamp(0.198f, 0.01f, 1.f);
    spectrum1.alpha = JonswapAlpha(9.81f, 100000.f, 0.5f);
    spectrum1.peakOmega = JonswapPeakFrequency(9.81f, 100000.f, 0.5f);
    spectrum1.gamma = 3.3;
    spectrum1.shortWavesFade = 0.01;

    spectrum2.scale = 0;
    spectrum2.angle = 0 / 180.0 * M_PI;
    spectrum2.spreadBlend = 1;
    spectrum2.swell = glm::clamp(1.f, 0.01f, 1.f);
    spectrum2.alpha = JonswapAlpha(9.81f, 300000.f, 1.f);
    spectrum2.peakOmega = JonswapPeakFrequency(9.81f, 300000.f, 1.f);
    spectrum2.gamma = 3.3;
    spectrum2.shortWavesFade = 0.01;

    waveGenData waveGenDataVar;
    waveGenDataVar.LengthScale = LengthScale;
    // waveGenDataVar.CutoffLow = 0.0001;
    // waveGenDataVar.CutoffHigh = 2.2f;
    waveGenDataVar.CutoffLow = CutoffLow;
    waveGenDataVar.CutoffHigh = CutoffHigh;
    waveGenDataVar.Size = 512;
    waveGenDataVar.spectrums[0] = spectrum1;
    waveGenDataVar.spectrums[1] = spectrum2;

    waveGenDataBuffers->writeToBuffer(&waveGenDataVar);
    waveGenDataBuffers->flush();
}

void WaveSpectrum::updateWaveParameters() {}

void WaveSpectrum::executePreCpS(FrameInfo frameInfo) {
    VkDescriptorSet descriptorSet[] = {waveGenDescriptorSets};

    lveCPipeline->bind(frameInfo.preProcessingCommandBuffer);

    SimplePushConstantData push{};
    push.resolution = glm::vec2(512, 512);
    vkCmdPushConstants(frameInfo.preProcessingCommandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(SimplePushConstantData), &push);

    vkCmdBindDescriptorSets(frameInfo.preProcessingCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1,
                            descriptorSet, 0, 0);

    vkCmdDispatch(frameInfo.preProcessingCommandBuffer, 512 / 32 + 1, 512 / 32 + 1, 1);
}

}  // namespace lve