#pragma once

#include "lve_frame_info.hpp"


namespace lve
{
  class LveIPostProcessing
  {
  public:
    virtual void executeCpS(FrameInfo frameInfo, VkDescriptorSet computeDescriptorSets) = 0;
    
  private:
  };
}