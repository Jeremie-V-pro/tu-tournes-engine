#pragma once

#include "lve_device.hpp"
#include "lve_c_pipeline.hpp"
#include "lve_frame_info.hpp"
#include "lve_Ipost_processing.hpp"

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace lve {
class SimpleComputeSystemR : public LveIPostProcessing{
public:

  SimpleComputeSystemR(LveDevice &device, VkDescriptorSetLayout textureSetLayout);
  ~SimpleComputeSystemR();

  void executeCpS(FrameInfo FrameInfo, VkDescriptorSet computeDescriptorSets, VkExtent2D extent) override;


private:
  void createPipelineLayout(VkDescriptorSetLayout textureSetLayout);
  void createPipeLine();


  LveDevice& lveDevice;
  std::unique_ptr<LveCPipeline> lveCPipeline;
  VkPipelineLayout pipelineLayout;
};
} // namespace lve