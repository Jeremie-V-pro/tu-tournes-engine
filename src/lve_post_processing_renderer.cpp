#include "lve_post_processing_renderer.hpp"
#include "lve_swap_chain.hpp"

namespace lve
{
  LvePostProcessingRenderer::LvePostProcessingRenderer(LveDevice &lveDevice, VkExtent2D windowExtent) : lveDevice{lveDevice}, windowExtent{windowExtent}
  {
    createTexture();
    createDescriptorPool();
    createDescriptorSet();
  }

  LvePostProcessingRenderer::~LvePostProcessingRenderer(){ 
  }

  void LvePostProcessingRenderer::createTexture(){
    textures.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
    {
      textures[i] = std::make_unique<LveTexture>(lveDevice, windowExtent.width, windowExtent.height);
    }
  }

  void LvePostProcessingRenderer::createDescriptorPool(){
    postprocessingPool = LveDescriptorPool::Builder(lveDevice)
                      .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                      .build();
  }

  void LvePostProcessingRenderer::createDescriptorSet(){
    texturesDescriptorSets.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
      VkDescriptorImageInfo imageDescriptorInfo1{};
      imageDescriptorInfo1.sampler = textures[i]->getSampler();
      imageDescriptorInfo1.imageView = textures[i]->getImageView();
      imageDescriptorInfo1.imageLayout = textures[i]->getImageLayout();

      VkDescriptorImageInfo imageDescriptorInfo2{};
      imageDescriptorInfo2.sampler = textures[i+1]->getSampler();
      imageDescriptorInfo2.imageView = textures[i+1]->getImageView();
      imageDescriptorInfo2.imageLayout = textures[i+1]->getImageLayout();

      VkDescriptorSet textureDescriptorSet1;
      VkDescriptorSet textureDescriptorSet2;
      LveDescriptorWriter(*LveDescriptorSetLayout::defaultTextureSetLayout, *postprocessingPool)
          .writeImage(0, &imageDescriptorInfo1)
          .writeImage(1, &imageDescriptorInfo2)
          .build(textureDescriptorSet1);
      LveDescriptorWriter(*LveDescriptorSetLayout::defaultTextureSetLayout, *postprocessingPool)
          .writeImage(0, &imageDescriptorInfo2)
          .writeImage(1, &imageDescriptorInfo1)
          .build(textureDescriptorSet2);
      texturesDescriptorSets[i] = std::make_pair(textureDescriptorSet1, textureDescriptorSet2);
    }

  }

  void LvePostProcessingRenderer::addPostProcessing(std::shared_ptr<LveIPostProcessing> postProcessing){
    postProcessings.push_back(postProcessing);
  }

  void LvePostProcessingRenderer::clearPostProcessings(){
    postProcessings.clear();
  }

  void LvePostProcessingRenderer::drawPostProcessings(FrameInfo frameInfo){
    
    for (int i = 0; i < postProcessings.size(); i++){
      postProcessings[i]->executeCpS(frameInfo, texturesDescriptorSets[frameInfo.frameIndex].first);
      std::swap(texturesDescriptorSets[frameInfo.frameIndex].first, texturesDescriptorSets[frameInfo.frameIndex].second);
    }
  }
}