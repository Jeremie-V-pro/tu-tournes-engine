#include "lve_post_processing_manager.hpp"

#include <vulkan/vulkan_core.h>

#include <iostream>

#include "lve_swap_chain.hpp"
#include "lve_utils.hpp"

namespace lve {
LvePostProcessingManager::LvePostProcessingManager(LveDevice &lveDevice, VkExtent2D windowExtent,
                                                   std::vector<VkImageView> depthImageViews,
                                                   std::vector<VkSampler> depthSamplers)
    : lveDevice{lveDevice}, windowExtent{windowExtent} {
    createTexture();

    createDescriptorPool();

    createDescriptorSet(depthImageViews, depthSamplers);

    createImageBarrier();

    createSyncObjects();
}

LvePostProcessingManager::LvePostProcessingManager(VkExtent2D windowExtent,
                                                   LvePostProcessingManager &postProcessingManager,
                                                   std::vector<VkImageView> depthImageViews,
                                                   std::vector<VkSampler> depthSamplers)
    : lveDevice{postProcessingManager.lveDevice},
      windowExtent{windowExtent},
      postProcessings{postProcessingManager.postProcessings} {
    createTexture();

    createDescriptorPool();

    createDescriptorSet(depthImageViews, depthSamplers);

    createImageBarrier();

    createSyncObjects();
}

LvePostProcessingManager::~LvePostProcessingManager() {
    for (size_t i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(lveDevice.device(), computeFinishedSemaphores[i], nullptr);
    }
}

void LvePostProcessingManager::createTexture() {
    textures.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2);
    for (int i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2; i++) {
        textures[i] = std::make_unique<LveTexture>(lveDevice, windowExtent.width, windowExtent.height);
    }
}

void LvePostProcessingManager::createDescriptorPool() {
    postprocessingPool =
        LveDescriptorPool::Builder(lveDevice)
            .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2 + 6)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LveSwapChain::MAX_FRAMES_IN_FLIGHT * 4)
            .build();
}

void LvePostProcessingManager::createDescriptorSet(std::vector<VkImageView> depthImageViews,
                                                   std::vector<VkSampler> depthSamplers) {
    texturesDescriptorSets.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2; i += 2) {
        VkDescriptorImageInfo imageDescriptorInfo1{};
        imageDescriptorInfo1.imageView = textures[i]->getImageView();
        imageDescriptorInfo1.imageLayout = textures[i]->getImageLayout();

        VkDescriptorImageInfo imageDescriptorInfo2{};
        imageDescriptorInfo2.imageView = textures[i + 1]->getImageView();
        imageDescriptorInfo2.imageLayout = textures[i + 1]->getImageLayout();

        VkDescriptorSet textureDescriptorSet1;
        VkDescriptorSet textureDescriptorSet2;
        LveDescriptorWriter(*LveDescriptorSetLayout::defaultPostProcessingTextureSetLayout, *postprocessingPool)
            .writeImage(0, &imageDescriptorInfo1)
            .writeImage(1, &imageDescriptorInfo2)
            .build(textureDescriptorSet1);

        LveDescriptorWriter(*LveDescriptorSetLayout::defaultPostProcessingTextureSetLayout, *postprocessingPool)
            .writeImage(0, &imageDescriptorInfo2)
            .writeImage(1, &imageDescriptorInfo1)
            .build(textureDescriptorSet2);
        texturesDescriptorSets[i / 2] = std::make_pair(textureDescriptorSet1, textureDescriptorSet2);
    }

    depthDescriptorSets.resize(depthSamplers.size());
    for (int i = 0; i < depthSamplers.size(); i++) {
        VkDescriptorImageInfo depthImageDescriptorInfo{};
        depthImageDescriptorInfo.sampler = depthSamplers[i];
        depthImageDescriptorInfo.imageView = depthImageViews[i];
        depthImageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        LveDescriptorWriter(*LveDescriptorSetLayout::depthTextureSetLayout, *postprocessingPool)
            .writeImage(0, &depthImageDescriptorInfo)
            .build(depthDescriptorSets[i]);
    }
}

void LvePostProcessingManager::addPostProcessing(std::shared_ptr<LveIPostProcessing> postProcessing) {
    postProcessings.push_back(postProcessing);
}

void LvePostProcessingManager::clearPostProcessings() { postProcessings.clear(); }

void LvePostProcessingManager::drawPostProcessings(FrameInfo frameInfo, VkImage swapChainImage, VkImage depthImage,
                                                   SynchronisationObjects &syncObjects) {
    copySwapChainImageToTexture(frameInfo, swapChainImage, textures[frameInfo.frameIndex * 2]->getTextureImage());

    VkImageMemoryBarrier transferDestImageBarrier{};
    transferDestImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferDestImageBarrier.srcAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    transferDestImageBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    transferDestImageBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    transferDestImageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    transferDestImageBarrier.image = depthImage;
    transferDestImageBarrier.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(frameInfo.postProcessingCommandBuffer, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &transferDestImageBarrier);

    int i;
    for (i = 0; i < postProcessings.size(); i++) {
        vkCmdPipelineBarrier(frameInfo.postProcessingCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &imagesBarriers[frameInfo.frameIndex * 2 + (i % 2)]);

        postProcessings[i]->executePostCpS(frameInfo, texturesDescriptorSets[frameInfo.frameIndex].first,
                                           depthDescriptorSets[frameInfo.swapChainImageIndex], windowExtent);

        std::swap(texturesDescriptorSets[frameInfo.frameIndex].first,
                  texturesDescriptorSets[frameInfo.frameIndex].second);
    }
    if (i % 2) {
        std::swap(texturesDescriptorSets[frameInfo.frameIndex].first,
                  texturesDescriptorSets[frameInfo.frameIndex].second);
    }
    copyTextureToSwapChainImage(frameInfo, swapChainImage,
                                textures[frameInfo.frameIndex * 2 + i % 2]->getTextureImage());

    transferDestImageBarrier = {};
    transferDestImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferDestImageBarrier.dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    transferDestImageBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    transferDestImageBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    transferDestImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    transferDestImageBarrier.image = depthImage;
    transferDestImageBarrier.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(frameInfo.postProcessingCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &transferDestImageBarrier);

    if (vkEndCommandBuffer(frameInfo.postProcessingCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore signalSemaphores[] = {computeFinishedSemaphores[frameInfo.frameIndex]};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frameInfo.postProcessingCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    submitInfo.waitSemaphoreCount = syncObjects.semaphores.size();
    submitInfo.pWaitSemaphores = syncObjects.semaphores.data();
    ;
    submitInfo.pWaitDstStageMask = waitStages;

    if (vkQueueSubmit(lveDevice.graphicsQueue(), 1, &submitInfo, syncObjects.fences[0]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit compute command buffer!");
    }
    syncObjects.fences.clear();
    syncObjects.semaphores.clear();
    syncObjects.semaphores.push_back(computeFinishedSemaphores[frameInfo.frameIndex]);
}

void LvePostProcessingManager::createSyncObjects() {
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

void LvePostProcessingManager::createImageBarrier() {
    imagesBarriers.clear();
    // loop in textures
    for (auto &texture : textures) {
        VkImageMemoryBarrier imagesBarrier{};
        imagesBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imagesBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        imagesBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imagesBarrier.image = texture->getTextureImage();
        imagesBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        imagesBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        imagesBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        imagesBarriers.push_back(imagesBarrier);
    }
}

void LvePostProcessingManager::copySwapChainImageToTexture(FrameInfo frameInfo, VkImage swapChainImage,
                                                           VkImage postprocessingImage) {
    VkImageMemoryBarrier transferDestImageBarrier{};
    transferDestImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferDestImageBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    transferDestImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferDestImageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    transferDestImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferDestImageBarrier.image = postprocessingImage;
    transferDestImageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkImageMemoryBarrier transferSwapChainImageBarrier{};
    transferSwapChainImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferSwapChainImageBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    transferSwapChainImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferSwapChainImageBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    transferSwapChainImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferSwapChainImageBarrier.image = swapChainImage;
    transferSwapChainImageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(frameInfo.postProcessingCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &transferDestImageBarrier);
    vkCmdPipelineBarrier(frameInfo.postProcessingCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &transferSwapChainImageBarrier);

    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = windowExtent.width;
    imageCopyRegion.extent.height = windowExtent.height;
    imageCopyRegion.extent.depth = 1;

    // Issue the copy command
    vkCmdCopyImage(frameInfo.postProcessingCommandBuffer, swapChainImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   postprocessingImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

    VkImageMemoryBarrier transferBackDestImageBarrier{};
    transferBackDestImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferBackDestImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBackDestImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    transferBackDestImageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBackDestImageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    transferBackDestImageBarrier.image = postprocessingImage;
    transferBackDestImageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(frameInfo.postProcessingCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &transferBackDestImageBarrier);
}

void LvePostProcessingManager::copyTextureToSwapChainImage(FrameInfo frameInfo, VkImage swapChainImage,
                                                           VkImage postprocessingImage) {
    VkImageMemoryBarrier transferDestImageBarrier{};
    transferDestImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferDestImageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
    transferDestImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferDestImageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    transferDestImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferDestImageBarrier.image = postprocessingImage;
    transferDestImageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkImageMemoryBarrier transferSwapChainImageBarrier{};
    transferSwapChainImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferSwapChainImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferSwapChainImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferSwapChainImageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferSwapChainImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferSwapChainImageBarrier.image = swapChainImage;
    transferSwapChainImageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(frameInfo.postProcessingCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &transferDestImageBarrier);
    vkCmdPipelineBarrier(frameInfo.postProcessingCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &transferSwapChainImageBarrier);

    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = windowExtent.width;
    imageCopyRegion.extent.height = windowExtent.height;
    imageCopyRegion.extent.depth = 1;

    // Issue the copy command
    vkCmdCopyImage(frameInfo.postProcessingCommandBuffer, postprocessingImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   swapChainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

    VkImageMemoryBarrier transferBackDestImageBarrier{};
    transferBackDestImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferBackDestImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBackDestImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    transferBackDestImageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBackDestImageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    transferBackDestImageBarrier.image = postprocessingImage;
    transferBackDestImageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkImageMemoryBarrier transferBackSwapchainImageBarrier{};
    transferBackSwapchainImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferBackSwapchainImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBackSwapchainImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBackSwapchainImageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBackSwapchainImageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    transferBackSwapchainImageBarrier.image = swapChainImage;
    transferBackSwapchainImageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(frameInfo.postProcessingCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &transferBackDestImageBarrier);
    vkCmdPipelineBarrier(frameInfo.postProcessingCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &transferBackSwapchainImageBarrier);
}

}  // namespace lve