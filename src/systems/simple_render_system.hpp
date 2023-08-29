#pragma once

#include "lve_camera.hpp"
#include "lve_device.hpp"
#include "lve_g_pipeline.hpp"
#include "lve_game_object.hpp"
#include "lve_frame_info.hpp"


#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace lve {
class SimpleRenderSystem {
public:


  SimpleRenderSystem(LveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout textureSetLayout);
  ~SimpleRenderSystem();

  SimpleRenderSystem(const LveWindow &) = delete;
  SimpleRenderSystem &operator=(const LveWindow &) = delete;

  void renderGameObjects(FrameInfo &frameInfo);

private:
  void createPipelineLayout(VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout textureSetLayout);
  void createPipeLine(VkRenderPass renderPass);


  LveDevice& lveDevice;
  std::unique_ptr<LveGPipeline> lveGPipeline;
  VkPipelineLayout pipelineLayout;
};
} // namespace lve