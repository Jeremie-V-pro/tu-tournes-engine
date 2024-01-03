#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>

#include "../lve_c_pipeline.hpp"
#include "../lve_g_pipeline.hpp"
#include "lve_utils.hpp"

namespace lve {
class PipelineBuilder {
   public:
    static VkPipelineLayout BuildPipeLineLayout(PipelineCreateInfo &pipelineCreateInfo);
    static std::unique_ptr<LveGPipeline> BuildGraphicsPipeline(PipelineCreateInfo &pipelineCreateInfo,
                                                               VkPipelineLayout pipelineLayout);

    static std::unique_ptr<LveCPipeline> BuildComputesPipeline(PipelineCreateInfo &pipelineCreateInfo,
                                                               VkPipelineLayout pipelineLayout);

   private:
};
}  // namespace lve