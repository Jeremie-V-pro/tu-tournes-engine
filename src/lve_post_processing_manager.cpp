#include "lve_post_processing_manager.hpp"
#include "lve_swap_chain.hpp"
#include <iostream>

namespace lve
{
  LvePostProcessingManager::LvePostProcessingManager(LveDevice &lveDevice, VkExtent2D windowExtent) : lveDevice{lveDevice}, windowExtent{windowExtent}
  {

    createTexture();

    createDescriptorPool();

    createDescriptorSet();
  }

  LvePostProcessingManager::LvePostProcessingManager(VkExtent2D windowExtent, LvePostProcessingManager &postProcessingManager) : lveDevice{postProcessingManager.lveDevice}, windowExtent{windowExtent}, postProcessings{postProcessingManager.postProcessings}, computeInFlightFences{postProcessingManager.computeInFlightFences}, computeFinishedSemaphores{postProcessingManager.computeFinishedSemaphores}
  {
    createTexture();

    createDescriptorPool();

    createDescriptorSet();
  }

  LvePostProcessingManager::~LvePostProcessingManager()
  {
  }

  void LvePostProcessingManager::createTexture()
  {
    textures.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2);
    for (int i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2; i++)
    {
      textures[i] = std::make_unique<LveTexture>(lveDevice, windowExtent.width, windowExtent.height);
    }
  }



  void LvePostProcessingManager::createDescriptorPool()
  {
    postprocessingPool = LveDescriptorPool::Builder(lveDevice)
                             .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2)
                             .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2)
                             .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2)
                             .build();
  }

  void LvePostProcessingManager::createDescriptorSet()
  {
    texturesDescriptorSets.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    std::cout << "A" << std::endl;
    for (int i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
    {

      VkDescriptorImageInfo imageDescriptorInfo1{};
      imageDescriptorInfo1.sampler = textures[i]->getSampler();
      imageDescriptorInfo1.imageView = textures[i]->getImageView();
      imageDescriptorInfo1.imageLayout = textures[i]->getImageLayout();

      VkDescriptorImageInfo imageDescriptorInfo2{};
      imageDescriptorInfo2.sampler = textures[i + 1]->getSampler();
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
      texturesDescriptorSets[i] = std::make_pair(textureDescriptorSet1, textureDescriptorSet2);
    }
  }

  void LvePostProcessingManager::addPostProcessing(std::shared_ptr<LveIPostProcessing> postProcessing)
  {
    postProcessings.push_back(postProcessing);
  }

  void LvePostProcessingManager::clearPostProcessings()
  {
    postProcessings.clear();
  }

  void LvePostProcessingManager::drawPostProcessings(FrameInfo frameInfo)
  {

    for (int i = 0; i < postProcessings.size(); i++)
    {
      postProcessings[i]->executeCpS(frameInfo, texturesDescriptorSets[frameInfo.frameIndex].first);
      std::swap(texturesDescriptorSets[frameInfo.frameIndex].first, texturesDescriptorSets[frameInfo.frameIndex].second);
    }
  }

  void LvePostProcessingManager::createSyncObjects()
  {
    computeInFlightFences->resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    computeFinishedSemaphores->resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    VkSemaphoreCreateInfo semaphoreInfo{};
semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

VkFenceCreateInfo fenceInfo{};
fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

for (size_t i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
   
    if (vkCreateSemaphore(lveDevice.device(), &semaphoreInfo, nullptr, &(*computeFinishedSemaphores)[i]) != VK_SUCCESS ||
        vkCreateFence(lveDevice.device(), &fenceInfo, nullptr, &(*computeInFlightFences)[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute synchronization objects for a frame!");
    }
}
  }

}