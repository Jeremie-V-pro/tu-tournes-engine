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
    

    void addPostProcessing(std::shared_ptr<LveIPostProcessing> postProcessing);
    
    void clearPostProcessings();

    void drawPostProcessings(FrameInfo frameInfo);

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

    std::shared_ptr<std::vector<VkFence>> computeInFlightFences;
    std::shared_ptr<std::vector<VkSemaphore>> computeFinishedSemaphores;
  };
}