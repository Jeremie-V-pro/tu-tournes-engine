#include "lve_post_processing_manager.hpp"
#include "lve_swap_chain.hpp"
#include <iostream>

namespace lve
{
  LvePostProcessingManager::LvePostProcessingManager(LveDevice &lveDevice, VkExtent2D windowExtent) : lveDevice{lveDevice}, windowExtent{windowExtent}
  {
    std::cout << "1" << std::endl;
    createTexture();
    std::cout << "2" << std::endl;
    createDescriptorPool();
    std::cout << "3" << std::endl;
    createDescriptorSet();
    std::cout << "4" << std::endl;
  }

  LvePostProcessingManager::~LvePostProcessingManager(){ 
  }

  void LvePostProcessingManager::createTexture(){
    textures.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2);
    for (int i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2; i++)
    {
      textures[i] = std::make_unique<LveTexture>(lveDevice, windowExtent.width, windowExtent.height);
    }
  }

   void LvePostProcessingManager::reCreateTexture(VkExtent2D windowExtent){
    this->windowExtent = windowExtent;

    
    textures.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2);
    for (int i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2; i++)
    {
      textures[i] = std::make_unique<LveTexture>(lveDevice, windowExtent.width, windowExtent.height);
    }
  }

  void LvePostProcessingManager::createDescriptorPool(){
    postprocessingPool = LveDescriptorPool::Builder(lveDevice)
                      .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .build();
  }

  void LvePostProcessingManager::createDescriptorSet(){
    texturesDescriptorSets.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    std::cout << "A" << std::endl;
    for (int i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
      std::cout << "début" << i << std::endl;

      VkDescriptorImageInfo imageDescriptorInfo1{};
      imageDescriptorInfo1.sampler = textures[i]->getSampler();
      imageDescriptorInfo1.imageView = textures[i]->getImageView();
      imageDescriptorInfo1.imageLayout = textures[i]->getImageLayout();
      std ::cout << "Quoi" << std::endl;
      VkDescriptorImageInfo imageDescriptorInfo2{};
      imageDescriptorInfo2.sampler = textures[i+1]->getSampler();
      imageDescriptorInfo2.imageView = textures[i+1]->getImageView();
      imageDescriptorInfo2.imageLayout = textures[i+1]->getImageLayout();
      std ::cout << "Feur" << std::endl;
      VkDescriptorSet textureDescriptorSet1;
      VkDescriptorSet textureDescriptorSet2;
      LveDescriptorWriter(*LveDescriptorSetLayout::defaultPostProcessingTextureSetLayout, *postprocessingPool)
          .writeImage(0, &imageDescriptorInfo1)
          .writeImage(1, &imageDescriptorInfo2)
          .build(textureDescriptorSet1);
      std ::cout << "Comment" << std::endl;
      LveDescriptorWriter(*LveDescriptorSetLayout::defaultPostProcessingTextureSetLayout, *postprocessingPool)
          .writeImage(0, &imageDescriptorInfo2)
          .writeImage(1, &imageDescriptorInfo1)
          .build(textureDescriptorSet2);
      texturesDescriptorSets[i] = std::make_pair(textureDescriptorSet1, textureDescriptorSet2);
      std ::cout << "dans coust" << std::endl;
      std::cout << "fin" << i << std::endl;
    }
     std::cout << "B" << std::endl;


  }

  void LvePostProcessingManager::addPostProcessing(std::shared_ptr<LveIPostProcessing> postProcessing){
    postProcessings.push_back(postProcessing);
  }

  void LvePostProcessingManager::clearPostProcessings(){
    postProcessings.clear();
  }

  void LvePostProcessingManager::drawPostProcessings(FrameInfo frameInfo){
    
    for (int i = 0; i < postProcessings.size(); i++){
      postProcessings[i]->executeCpS(frameInfo, texturesDescriptorSets[frameInfo.frameIndex].first);
      std::swap(texturesDescriptorSets[frameInfo.frameIndex].first, texturesDescriptorSets[frameInfo.frameIndex].second);
    }
  }


}