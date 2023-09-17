#pragma once

#include "lve_device.hpp"
#include "lve_window.hpp"
#include "lve_game_object.hpp"
#include "lve_renderer.hpp"
#include "lve_descriptor.hpp"
#include "lve_texture.hpp"

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
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

  LveWindow lveWindow{WIDTH, HEIGHT, "hello Vulkan"};
  LveDevice lveDevice{lveWindow};
  LveRenderer lveRenderer{lveWindow, lveDevice};
  
  //l'ordre de déclaration compte
  std::unique_ptr<LveDescriptorPool> globalPool{};
  std::unique_ptr<LveTexture> texture{};

  std::shared_ptr<LveTexture> texturesrc;
  std::shared_ptr<LveTexture> texturedst;
  VkDescriptorSet textureDescriptorSet;
  
  std::unique_ptr<LveDescriptorPool> texturePool;
  std::unique_ptr<LveDescriptorSetLayout> testlayout;

  LveGameObject::Map gameObjects;
};
} // namespace lve