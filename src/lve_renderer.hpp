#pragma once

#include "lve_device.hpp"
#include "lve_swap_chain.hpp"
#include "lve_window.hpp"
#include "lve_post_processing_manager.hpp"

#include <cstdint>
#include <memory>
#include <cassert>
namespace lve {
class LveRenderer {
public:

  LveRenderer(LveWindow &window, LveDevice &device);
  ~LveRenderer();



  VkRenderPass getSwapChainRenderPass() const {return lveSwapChain->getRenderPass();}
  float getAspectRatio() const {return lveSwapChain->extentAspectRatio();}
  bool isFrameInProgress() const { return isFrameStarted;}

  VkCommandBuffer getCurrentCommandBuffer() const{
    assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
    return  commandBuffers[currentFrameIndex];
  }

  VkCommandBuffer getCurrentComputeCommandBuffer() const{
    assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
    return  computeCommandBuffers[currentFrameIndex];
  }

  int getFrameIndex() const{
    assert(isFrameStarted && "cannot get frame index when not frame in progress");
    return currentFrameIndex;
  }

  VkCommandBuffer beginFrame();
  void endFrame();
  void presentFrame();
  void renderPostProssessingEffects(FrameInfo frameInfo);
  void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
  void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
  void addPostProcessingEffect(std::shared_ptr<LveIPostProcessing> postProcessing);

private:


  void createCommandBuffers();
  void freeCommandBuffers();
  void recreateSwapChain();

  LveWindow& lveWindow;
  LveDevice& lveDevice;
  std::unique_ptr<LveSwapChain> lveSwapChain;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkCommandBuffer> computeCommandBuffers;

  std::unique_ptr<LvePostProcessingManager> postProcessingManager;

  std::uint32_t currentImageIndex;
  int currentFrameIndex{0};
  bool isFrameStarted{false};
};
} // namespace lve