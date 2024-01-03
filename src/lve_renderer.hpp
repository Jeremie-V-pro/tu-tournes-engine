#pragma once

#include <cassert>
#include <cstdint>
#include <memory>

#include "lve_device.hpp"
#include "lve_post_processing_manager.hpp"
#include "lve_pre_processing_manager.hpp"
#include "lve_swap_chain.hpp"
#include "lve_utils.hpp"
#include "lve_window.hpp"
#include "systems/lve_Ipre_processing.hpp"
namespace lve {
class LveRenderer {
   public:
    LveRenderer(LveWindow &window, LveDevice &device);
    ~LveRenderer();

    VkRenderPass getSwapChainRenderPass() const { return lveSwapChain->getRenderPass(); }
    float getAspectRatio() const { return lveSwapChain->extentAspectRatio(); }
    bool isFrameInProgress() const { return isFrameStarted; }

    VkCommandBuffer getCurrentCommandBuffer() const {
        assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
        return commandBuffers[currentFrameIndex];
    }

    VkCommandBuffer getPostProcessingCommandBuffer() const {
        assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
        return postProcessingBuffers[currentFrameIndex];
    }

    VkCommandBuffer getPreProcessingCommandBuffer() const {
        assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
        return preProcessingBuffers[currentFrameIndex];
    }

    int getFrameIndex() const {
        assert(isFrameStarted && "cannot get frame index when not frame in progress");
        return currentFrameIndex;
    }

    int getSwapchainFrameIndex() const {
        assert(isFrameStarted && "cannot get frame index when not frame in progress");
        return currentImageIndex;
    }

    bool startRendering(SynchronisationObjects &syncObjects);
    VkCommandBuffer beginFrame(SynchronisationObjects &syncObjects);
    void endFrame(SynchronisationObjects &syncObjects);
    void presentFrame(SynchronisationObjects &syncObjects);
    void renderPostProssessingEffects(FrameInfo frameInfo, SynchronisationObjects &syncObjects);
    void executePreProssessingEffects(FrameInfo frameInfo, SynchronisationObjects &syncObjects);
    void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void addPostProcessingEffect(std::shared_ptr<LveIPostProcessing> postProcessing);
    void addPreProcessingEffect(std::shared_ptr<LveIPreProcessing> preProcessing);

   private:
    void createCommandBuffers();
    void freeCommandBuffers();
    void recreateSwapChain();

    LveWindow &lveWindow;
    LveDevice &lveDevice;
    std::unique_ptr<LveSwapChain> lveSwapChain;
    std::unique_ptr<LvePostProcessingManager> postProcessingManager;
    std::unique_ptr<LvePreProcessingManager> preProcessingManager;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> preProcessingBuffers;
    std::vector<VkCommandBuffer> postProcessingBuffers;

    std::uint32_t currentImageIndex;
    int currentFrameIndex{0};
    bool isFrameStarted{false};
};
}  // namespace lve