#pragma once

#include <memory>

#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_utils.hpp"
#include "systems/lve_Ipre_processing.hpp"

namespace lve {
class LvePreProcessingManager {
   public:
    LvePreProcessingManager(LveDevice &deviceRef);
    ~LvePreProcessingManager();

    void createSyncObjects();

    void addPreProcessing(std::shared_ptr<LveIPreProcessing> preProcessing);

    void clearPreProcessings();

    void executePreprocessing(FrameInfo frameInfo, SynchronisationObjects &syncObjects);

    VkSemaphore getComputeSemaphore(int frame_index) const { return computeFinishedSemaphores[frame_index]; };

   private:
    LveDevice &lveDevice;
    std::vector<std::shared_ptr<LveIPreProcessing>> preProcessings;
    std::vector<VkSemaphore> computeFinishedSemaphores;
};
}  // namespace lve