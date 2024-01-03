#pragma once

#include <vulkan/vulkan_core.h>

#include <functional>
#include <string>
#include <vector>

#include "lve_device.hpp"

namespace lve {

// from: https://stackoverflow.com/a/57595105
template <typename T, typename... Rest>
void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hashCombine(seed, rest), ...);
};

enum LvePipeLineType {
    LvePipeLineTypeRender = 0,
    LvePipeLineTypeCompute = 1,
};

enum LvePipelIneFunctionnality {
    None = 0,
    Transparancy = 1,
};

struct PipelineCreateInfo {
    LveDevice& device;
    LvePipeLineType type;
    std::vector<VkDescriptorSetLayout> SetLayouts;
    std::vector<std::string> shaderPaths;
    uint32_t pushConstantRangeSize = 0;
    LvePipelIneFunctionnality functionnality = LvePipelIneFunctionnality::None;
    VkRenderPass renderPass;
};

struct SynchronisationObjects {
    std::vector<VkSemaphore> semaphores;
    std::vector<VkFence> fences;
};

}  // namespace lve