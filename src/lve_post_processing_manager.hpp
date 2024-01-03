#pragma once

#include "lve_device.hpp"
#include "lve_texture.hpp"
#include "systems/lve_Ipost_processing.hpp"
#include "lve_frame_info.hpp"
#include "lve_utils.hpp"

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace lve
{
  class LvePostProcessingManager
  {
  public:
    LvePostProcessingManager(LveDevice &deviceRef, VkExtent2D windowExtent, std::vector<VkImageView> depthImageViews, std::vector<VkSampler> depthSamplers);
    LvePostProcessingManager(VkExtent2D windowExtent,LvePostProcessingManager &postProcessingManager, std::vector<VkImageView> depthImageViews, std::vector<VkSampler> depthSamplers);
    ~LvePostProcessingManager();
    
    void createTexture();
    void createDescriptorPool();
    void createDescriptorSet(std::vector<VkImageView> depthImageViews, std::vector<VkSampler> depthSamplers);
    void createSyncObjects();
    void createImageBarrier();

    void addPostProcessing(std::shared_ptr<LveIPostProcessing> postProcessing);
    
    void clearPostProcessings();

    void drawPostProcessings(FrameInfo frameInfo, VkImage swapChainImage, VkImage depthImage, SynchronisationObjects &syncObjects);

    void copySwapChainImageToTexture(FrameInfo frameInfo, VkImage swapChainImage, VkImage postprocessingImage);

    void copyTextureToSwapChainImage(FrameInfo frameInfo, VkImage swapChainImage, VkImage postprocessingImage);

    VkSemaphore getComputeSemaphore(int frame_index) const {return computeFinishedSemaphores[frame_index];};



  private:
    LveDevice &lveDevice;

    VkExtent2D windowExtent;
    std::unique_ptr<LveDescriptorPool> postprocessingPool;
    std::vector<std::unique_ptr<LveTexture>> textures;
    std::vector<std::pair<VkDescriptorSet, VkDescriptorSet>> texturesDescriptorSets;
    std::vector<std::shared_ptr<LveIPostProcessing>> postProcessings;
    std::vector<VkImageMemoryBarrier> imagesBarriers;
    std::vector<VkSemaphore> computeFinishedSemaphores;
    std::vector<VkDescriptorSet> depthDescriptorSets;
    
  };
}