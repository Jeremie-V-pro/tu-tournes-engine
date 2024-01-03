#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

#include "lve_descriptor.hpp"
#include "lve_device.hpp"
#include "lve_game_object.hpp"
#include "lve_renderer.hpp"
#include "lve_texture.hpp"
#include "lve_utils.hpp"
#include "lve_window.hpp"
#include "systems/computesSystems/waveGenerationSystem.hpp"
namespace lve {
class FirstApp {
   public:
    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;

    FirstApp();
    ~FirstApp();

    FirstApp(const LveWindow &) = delete;
    FirstApp &operator=(const LveWindow &) = delete;
    void run();

   private:
    void loadGameObjects();

    LveWindow lveWindow{WIDTH, HEIGHT, "TutournesEgine v0.1"};
    LveDevice lveDevice{lveWindow};
    LveRenderer lveRenderer{lveWindow, lveDevice};
    SynchronisationObjects syncObjects;

    // l'ordre de déclaration compte
    std::unique_ptr<LveDescriptorPool> globalPool{};
    std::shared_ptr<LveTexture> display;
    std::shared_ptr<LveTexture> derivatives;
    std::shared_ptr<LveTexture> turbu;
    std::shared_ptr<WaveGen> waveGen1;
    std::shared_ptr<WaveGen> waveGen2;
    std::shared_ptr<WaveGen> waveGen3;
    unsigned int waterId;
    std::shared_ptr<LveGameObject> sun;
    LveGameObject::Map gameObjects;
    glm::vec3 sunOrientation;
};
}  // namespace lve