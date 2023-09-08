#pragma once

#include "lve_device.hpp"
#include "lve_texture.hpp"
#include "lve_Ipost_processing.hpp"
#include "lve_frame_info.hpp"

#include <memory>

namespace lve
{
  class LvePostProcessingRenderer
  {
  public:
    LvePostProcessingRenderer(LveDevice &deviceRef, VkExtent2D windowExtent);
    ~LvePostProcessingRenderer();

    LvePostProcessingRenderer(const LvePostProcessingRenderer &) = delete;
    LvePostProcessingRenderer &operator=(const LvePostProcessingRenderer &) = delete;
    
    void createTexture();
    void createDescriptorPool();
    void createDescriptorSet();
    

    void addPostProcessing(std::shared_ptr<LveIPostProcessing> postProcessing);
    
    void clearPostProcessings();

    void drawPostProcessings(FrameInfo frameInfo);

  private:
    LveDevice &lveDevice;

    VkExtent2D windowExtent;

    std::unique_ptr<LveDescriptorPool> postprocessingPool;
    std::vector<std::unique_ptr<LveTexture>> textures;
    std::vector<std::pair<VkDescriptorSet, VkDescriptorSet>> texturesDescriptorSets;
    std::unique_ptr<VkDescriptorSetLayout> descriptorSetLayout;
    std::vector<std::shared_ptr<LveIPostProcessing>> postProcessings;
  };
}