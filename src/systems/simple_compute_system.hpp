#pragma once

#include "lve_camera.hpp"
#include "lve_device.hpp"
#include "lve_c_pipeline.hpp"
#include "lve_game_object.hpp"
#include "lve_frame_info.hpp"


#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace lve {
class SimpleComputeSystem {
public:

  SimpleComputeSystem(LveDevice &device, VkDescriptorSetLayout textureSetLayout);
  ~SimpleComputeSystem();

  SimpleComputeSystem(const LveWindow &) = delete;
  SimpleComputeSystem &operator=(const LveWindow &) = delete;

  void renderGameObjects(FrameInfo &frameInfo);
  void executeCpS(VkCommandBuffer& commandBuffer, VkDescriptorSet computeDescriptorSets);


private:
  void createPipelineLayout(VkDescriptorSetLayout textureSetLayout);
  void createPipeLine();


  LveDevice& lveDevice;
  std::unique_ptr<LveCPipeline> lveCPipeline;
  VkPipelineLayout pipelineLayout;
};
} // namespace lve