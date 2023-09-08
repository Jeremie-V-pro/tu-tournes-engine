#pragma once

#include "lve_Ipost_processing.hpp"


namespace lve
{
  class LveIPostProcessing
  {
  public:
    virtual void executeCpS(FrameInfo frameInfo, VkDescriptorSet computeDescriptorSets) = 0;
    
  private:
  };
}