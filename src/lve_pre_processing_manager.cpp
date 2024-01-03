#include "lve_pre_processing_manager.hpp"

#include <iostream>

#include "lve_swap_chain.hpp"

namespace lve {
LvePreProcessingManager::LvePreProcessingManager(LveDevice &lveDevice) : lveDevice{lveDevice} { createSyncObjects(); }

LvePreProcessingManager::~LvePreProcessingManager() {
    for (size_t i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(lveDevice.device(), computeFinishedSemaphores[i], nullptr);
    }
}

void LvePreProcessingManager::addPreProcessing(std::shared_ptr<LveIPreProcessing> postProcessing) {
    preProcessings.push_back(postProcessing);
}

void LvePreProcessingManager::clearPreProcessings() { preProcessings.clear(); }

void LvePreProcessingManager::executePreprocessing(FrameInfo frameInfo, SynchronisationObjects &syncObjects) {
    int i;
    for (i = 0; i < preProcessings.size(); i++) {
        preProcessings[i]->executePreCpS(frameInfo);
    }

    if (vkEndCommandBuffer(frameInfo.preProcessingCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore signalSemaphores[] = {computeFinishedSemaphores[frameInfo.frameIndex]};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frameInfo.preProcessingCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    submitInfo.waitSemaphoreCount = syncObjects.semaphores.size();
    submitInfo.pWaitSemaphores = syncObjects.semaphores.data();
    submitInfo.pWaitDstStageMask = waitStages;

    if (vkQueueSubmit(lveDevice.graphicsQueue(), 1, &submitInfo, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit compute command buffer!");
    }
    syncObjects.semaphores.clear();
    syncObjects.semaphores.push_back(computeFinishedSemaphores[frameInfo.frameIndex]);
}

void LvePreProcessingManager::createSyncObjects() {
    computeFinishedSemaphores.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (size_t i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(lveDevice.device(), &semaphoreInfo, nullptr, &computeFinishedSemaphores[i]) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create compute synchronization objects for a frame!");
        }
    }
}
}  // namespace lve