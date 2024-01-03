#pragma once

#include "lve_frame_info.hpp"

namespace lve {
class LveIPostProcessing {
   public:
    virtual void executePostCpS(FrameInfo frameInfo, VkDescriptorSet computeDescriptorSets,
                                VkDescriptorSet depthDescriptorSets, VkExtent2D extent) = 0;

   private:
};
}  // namespace lve