#pragma once

#include "lve_frame_info.hpp"

namespace lve {
class LveIPreProcessing {
   public:
    virtual void executePreCpS(FrameInfo frameInfo) = 0;

   private:
};
}  // namespace lve