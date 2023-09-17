#pragma once

#include "lve_frame_info.hpp"


namespace lve
{
  class LveIPostProcessing
  {
  public:
    virtual void executeCpS(FrameInfo frameInfo, VkDescriptorSet computeDescriptorSets, VkExtent2D extent) = 0;
    
  private:
  };
}