#pragma once

#include "lve_device.hpp"
#include "lve_texture.hpp"
#include "lve_Ipost_processing.hpp"
#include "lve_frame_info.hpp"

#include <memory>

namespace lve
{
  class LvePostProcessingManager
  {
  public:
    LvePostProcessingManager(LveDevice &deviceRef, VkExtent2D windowExtent);
    LvePostProcessingManager(VkExtent2D windowExtent,LvePostProcessingManager &postProcessingManager);
    ~LvePostProcessingManager();
    
    void createTexture();
    void createDescriptorPool();
    void createDescriptorSet();
    void createSyncObjects();
    void createImageBarrier();

    void addPostProcessing(std::shared_ptr<LveIPostProcessing> postProcessing);
    
    void clearPostProcessings();

    void drawPostProcessings(FrameInfo frameInfo, VkImage swapChainImage, VkSemaphore renderFinishedSemaphore, VkFence fence);

    void copySwapChainImageToTexture(FrameInfo frameInfo, VkImage swapChainImage, VkImage postprocessingImage);

    void copyTextureToSwapChainImage(FrameInfo frameInfo, VkImage swapChainImage, VkImage postprocessingImage);

    VkSemaphore getComputeSemaphore(int frame_index) const {return computeFinishedSemaphores[frame_index];};

    //const function that returns vector "postProcessings"
    std::vector<std::shared_ptr<LveIPostProcessing>>& getPostProcessings() {
        return postProcessings;
    }


  private:
    LveDevice &lveDevice;

    VkExtent2D windowExtent;

    std::unique_ptr<LveDescriptorPool> postprocessingPool;
    std::vector<std::unique_ptr<LveTexture>> textures;
    std::vector<std::pair<VkDescriptorSet, VkDescriptorSet>> texturesDescriptorSets;
    std::vector<std::shared_ptr<LveIPostProcessing>> postProcessings;
    std::vector<VkImageMemoryBarrier> imagesBarriers;
    std::vector<VkSemaphore> computeFinishedSemaphores;
    
  };
}