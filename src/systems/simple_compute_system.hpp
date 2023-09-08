#pragma once

#include "lve_camera.hpp"
#include "lve_device.hpp"
#include "lve_c_pipeline.hpp"
#include "lve_game_object.hpp"
#include "lve_frame_info.hpp"
#include "lve_Ipost_processing.hpp"

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace lve {
class SimpleComputeSystem : public LveIPostProcessing{
public:

  SimpleComputeSystem(LveDevice &device, VkDescriptorSetLayout textureSetLayout);
  ~SimpleComputeSystem();

  SimpleComputeSystem(const LveWindow &) = delete;
  SimpleComputeSystem &operator=(const LveWindow &) = delete;

  void executeCpS(FrameInfo FrameInfo, VkDescriptorSet computeDescriptorSets);


private:
  void createPipelineLayout(VkDescriptorSetLayout textureSetLayout);
  void createPipeLine();


  LveDevice& lveDevice;
  std::unique_ptr<LveCPipeline> lveCPipeline;
  VkPipelineLayout pipelineLayout;
};
} // namespace lve