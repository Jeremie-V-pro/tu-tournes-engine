#pragma once
#include <vulkan/vulkan.h>

#include <glm/fwd.hpp>

#include "lve_camera.hpp"
#include "lve_game_object.hpp"

namespace lve {

#define MAX_LIGHTS 10

struct PointLight {
    glm::vec4 position{};  // ignore w
    glm::vec4 color{};     // w is intensity
};

struct GlobalUbo {
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};
    glm::mat4 inverseView{1.f};
    glm::vec4 sunDirection{0.f, -1.f, 0.f, 1.f};
    glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};  // w is intensity
    PointLight pointLights[MAX_LIGHTS];
    int numLights;
};

struct FrameInfo {
    int frameIndex;
    int swapChainImageIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    VkCommandBuffer preProcessingCommandBuffer;
    VkCommandBuffer postProcessingCommandBuffer;
    LveCamera &camera;
    VkDescriptorSet globalDescriptorSet;
    LveGameObject::Map &gameObjects;
};

}  // namespace lve