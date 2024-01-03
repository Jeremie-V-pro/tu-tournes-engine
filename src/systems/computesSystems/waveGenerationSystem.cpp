#include "waveGenerationSystem.hpp"

#include <vulkan/vulkan_core.h>

#include <vector>

#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_swap_chain.hpp"
#include "lve_texture.hpp"
#include "systems/computesSystems/waveGenerationSystems/wave_spectrum.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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

WaveGen::WaveGen(LveDevice &device, float LengthScale, float CutoffLow, float CutoffHigh) : lveDevice{device} {
    createTextures();
    waveTextureGenerator = std::make_unique<WaveSpectrum>(lveDevice, 512, 512, spectrumTexture, waveDataTexture,
                                                          LengthScale, CutoffLow, CutoffHigh);

    waveConjugate = std::make_unique<WaveConjugate>(lveDevice, 512, 512, spectrumTexture, spectrumConjugateTexture);

    waveVertIFFTDxDz = std::make_unique<WaveVertIFFT>(lveDevice, 512, 512, DxDz, spectrumTextureCopy1, preComputeData);
    waveHorIFFTDxDz = std::make_unique<WaveHorIFFT>(lveDevice, 512, 512, DxDz, spectrumTextureCopy1, preComputeData);

    waveVertIFFTDyDxz =
        std::make_unique<WaveVertIFFT>(lveDevice, 512, 512, DyDxz, spectrumTextureCopy1, preComputeData);
    waveHorIFFTDyDxz = std::make_unique<WaveHorIFFT>(lveDevice, 512, 512, DyDxz, spectrumTextureCopy1, preComputeData);

    waveVertIFFTDyxDyz =
        std::make_unique<WaveVertIFFT>(lveDevice, 512, 512, DyxDyz, spectrumTextureCopy1, preComputeData);
    waveHorIFFTDyxDyz =
        std::make_unique<WaveHorIFFT>(lveDevice, 512, 512, DyxDyz, spectrumTextureCopy1, preComputeData);

    waveVertIFFTDxxDzz =
        std::make_unique<WaveVertIFFT>(lveDevice, 512, 512, DxxDzz, spectrumTextureCopy1, preComputeData);
    waveHorIFFTDxxDzz =
        std::make_unique<WaveHorIFFT>(lveDevice, 512, 512, DxxDzz, spectrumTextureCopy1, preComputeData);

    wavePermuteDxDz = std::make_unique<WavePermute>(lveDevice, 512, 512, DxDz);
    wavePermuteDyDxz = std::make_unique<WavePermute>(lveDevice, 512, 512, DyDxz);
    wavePermuteDyxDyz = std::make_unique<WavePermute>(lveDevice, 512, 512, DyxDyz);
    wavePermuteDxxDzz = std::make_unique<WavePermute>(lveDevice, 512, 512, DxxDzz);

    waveMerge = std::make_unique<WaveMerge>(lveDevice, 512, 512, DxDz, DyDxz, DyxDyz, DxxDzz, displacement, derivatives,
                                            turbulence);

    waveTimeUpdate = std::make_unique<WaveTimeUpdate>(lveDevice, 512, 512, DxDz, DyDxz, DyxDyz, DxxDzz,
                                                      spectrumConjugateTexture, waveDataTexture);
}
WaveGen::~WaveGen() {}

std::vector<float> WaveGen::loadPrecomputeData() {
    const std::string filePath = "textures/precomputeData.csv";

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

void WaveGen::createTextures() {
    spectrumTextureCopy1.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    DxDz.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    DyDxz.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    DyxDyz.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    DxxDzz.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    displacement.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    derivatives.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    turbulence.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);

    spectrumTexture = std::make_shared<LveTexture>(lveDevice, 512, 512, std::vector<uint32_t>(512 * 512 * 2, 0).data(),
                                                   2, VK_FORMAT_R32G32_SFLOAT);

    waveDataTexture = std::make_shared<LveTexture>(lveDevice, 512, 512, std::vector<uint32_t>(512 * 512 * 4, 0).data(),
                                                   4, VK_FORMAT_R32G32B32A32_SFLOAT);

    preComputeData =
        std::make_shared<LveTexture>(lveDevice, 9, 512, loadPrecomputeData().data(), 4, VK_FORMAT_R32G32B32A32_SFLOAT);
    spectrumConjugateTexture = std::make_shared<LveTexture>(
        lveDevice, 512, 512, std::vector<uint32_t>(512 * 512 * 4, 0).data(), 4, VK_FORMAT_R32G32B32A32_SFLOAT);
    for (int i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        spectrumTextureCopy1[i] = std::make_shared<LveTexture>(
            lveDevice, 512, 512, std::vector<uint32_t>(512 * 512 * 2, 0).data(), 2, VK_FORMAT_R32G32_SFLOAT);

        DxDz[i] = std::make_shared<LveTexture>(lveDevice, 512, 512, std::vector<uint32_t>(512 * 512 * 2, 0).data(), 2,
                                               VK_FORMAT_R32G32_SFLOAT);

        DyDxz[i] = std::make_shared<LveTexture>(lveDevice, 512, 512, std::vector<uint32_t>(512 * 512 * 2, 0).data(), 2,
                                                VK_FORMAT_R32G32_SFLOAT);

        DyxDyz[i] = std::make_shared<LveTexture>(lveDevice, 512, 512, std::vector<uint32_t>(512 * 512 * 2, 0).data(), 2,
                                                 VK_FORMAT_R32G32_SFLOAT);

        DxxDzz[i] = std::make_shared<LveTexture>(lveDevice, 512, 512, std::vector<uint32_t>(512 * 512 * 2, 0).data(), 2,
                                                 VK_FORMAT_R32G32_SFLOAT);

        displacement[i] = std::make_shared<LveTexture>(
            lveDevice, 512, 512, std::vector<uint32_t>(512 * 512 * 4, 0).data(), 4, VK_FORMAT_R32G32B32A32_SFLOAT);

        derivatives[i] = std::make_shared<LveTexture>(
            lveDevice, 512, 512, std::vector<uint32_t>(512 * 512 * 4, 0).data(), 4, VK_FORMAT_R32G32B32A32_SFLOAT);

        turbulence[i] = std::make_shared<LveTexture>(
            lveDevice, 512, 512, std::vector<uint32_t>(512 * 512 * 4, 0).data(), 4, VK_FORMAT_R32G32B32A32_SFLOAT);
    }
}

void createPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStage,
                           VkPipelineStageFlags dstStage) {
    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;  // Accès en écriture
    memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;   // Accès en lecture

    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void WaveGen::executePreCpS(FrameInfo FrameInfo) {
    if (true) {
        // DataIsUpdate = false;
        waveTextureGenerator->executePreCpS(FrameInfo);
        createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        waveConjugate->executePreCpS(FrameInfo);
        createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        DataIsUpdate = false;
    }

    LveTexture::copyTexture(FrameInfo.preProcessingCommandBuffer, spectrumTexture,
                            spectrumTextureCopy1[FrameInfo.frameIndex]);

    createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    waveTimeUpdate->executePreCpS(FrameInfo);

    bool pingPong = false;
    int logSize = 9;
    for (int i = 0; i < logSize; i++) {
        pingPong = !pingPong;
        createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        waveHorIFFTDxDz->executePreCpS(FrameInfo, pingPong, i);
    }
    for (int i = 0; i < logSize; i++) {
        pingPong = !pingPong;
        createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        waveVertIFFTDxDz->executePreCpS(FrameInfo, pingPong, i);
    }
    createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    wavePermuteDxDz->executePreCpS(FrameInfo);

    pingPong = false;
    logSize = 9;
    for (int i = 0; i < logSize; i++) {
        pingPong = !pingPong;
        createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        waveHorIFFTDyDxz->executePreCpS(FrameInfo, pingPong, i);
    }
    for (int i = 0; i < logSize; i++) {
        pingPong = !pingPong;
        createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        waveVertIFFTDyDxz->executePreCpS(FrameInfo, pingPong, i);
    }
    createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    wavePermuteDyDxz->executePreCpS(FrameInfo);

    pingPong = false;
    logSize = 9;
    for (int i = 0; i < logSize; i++) {
        pingPong = !pingPong;
        createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        waveHorIFFTDyxDyz->executePreCpS(FrameInfo, pingPong, i);
    }
    for (int i = 0; i < logSize; i++) {
        pingPong = !pingPong;
        createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        waveVertIFFTDyxDyz->executePreCpS(FrameInfo, pingPong, i);
    }
    createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    wavePermuteDyxDyz->executePreCpS(FrameInfo);

    pingPong = false;
    logSize = 9;
    for (int i = 0; i < logSize; i++) {
        pingPong = !pingPong;
        createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        waveHorIFFTDxxDzz->executePreCpS(FrameInfo, pingPong, i);
    }
    for (int i = 0; i < logSize; i++) {
        pingPong = !pingPong;
        createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        waveVertIFFTDxxDzz->executePreCpS(FrameInfo, pingPong, i);
    }
    createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    wavePermuteDxxDzz->executePreCpS(FrameInfo);
    createPipelineBarrier(FrameInfo.preProcessingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    waveMerge->executePreCpS(FrameInfo);
}
}  // namespace lve