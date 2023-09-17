#include "first_app.hpp"

#include "keyboard_mouvement_controller.hpp"
#include "lve_camera.hpp"
#include "lve_descriptor.hpp"
#include "lve_game_object.hpp"
#include "lve_swap_chain.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/point_light_system.hpp"
#include "systems/simple_compute_system.hpp"
#include "lve_buffer.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <iostream>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <array>
#include <chrono>
#include <memory>
#include <stdexcept>

#include "lve_texture.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace lve
{

  FirstApp::FirstApp()
  {

    std::cout << "CACA" << std::endl;
    globalPool = LveDescriptorPool::Builder(lveDevice)
                     .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                     .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                     .build();

    // Set default descriptor layout
    std::shared_ptr<LveDescriptorSetLayout::Builder> setLayoutBuilder = std::make_shared<LveDescriptorSetLayout::Builder>(lveDevice);
    setLayoutBuilder->addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    LveDescriptorSetLayout::defaultTextureSetLayout = setLayoutBuilder->build();

    setLayoutBuilder = std::make_shared<LveDescriptorSetLayout::Builder>(lveDevice);
    setLayoutBuilder->addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
    setLayoutBuilder->addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
    LveDescriptorSetLayout::defaultPostProcessingTextureSetLayout = setLayoutBuilder->build();
    
    loadGameObjects();
  }

  FirstApp::~FirstApp() {}

  void FirstApp::run()
  {

    std::cout << "FirstApp::run()" << std::endl;

    // Création des uniform buffer pour chaques frames
    std::vector<std::unique_ptr<LveBuffer>> uboBuffers(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++)
    {
      uboBuffers[i] = std::make_unique<LveBuffer>(
          lveDevice, sizeof(GlobalUbo),
          1,
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      uboBuffers[i]->map();
    }

    // Création du layout pour les uniform buffer
    auto globalSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
                               .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
                               .build();

    // Création des descriptor sets pour les uniform buffer
    std::vector<VkDescriptorSet> globalDescriptorSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++)
    {
      auto bufferInfo = uboBuffers[i]->descriptorInfo();
      LveDescriptorWriter(*globalSetLayout, *globalPool)
          .writeBuffer(0, &bufferInfo)
          .build(globalDescriptorSets[i]);
    }

    // initialisation du system de rendu simple
    SimpleRenderSystem simpleRenderSystem{lveDevice,
                                          lveRenderer.getSwapChainRenderPass(),
                                          globalSetLayout->getDescriptorSetLayout(),
                                          LveDescriptorSetLayout::defaultTextureSetLayout->getDescriptorSetLayout()};

    // initialisation du system de rendu des luimères
    PointLightSystem pointLightSystem{lveDevice,
                                      lveRenderer.getSwapChainRenderPass(),
                                      globalSetLayout->getDescriptorSetLayout()};

    std::shared_ptr<SimpleComputeSystem> simpleComputeSystem = std::make_shared<SimpleComputeSystem>(lveDevice,
                                            LveDescriptorSetLayout::defaultPostProcessingTextureSetLayout->getDescriptorSetLayout());
    lveRenderer.addPostProcessingEffect(simpleComputeSystem);
    
    LveCamera camera{};
    // camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.5, 0.f, 1.f));
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

    auto viewerObject = LveGameObject::createGameObject();
    viewerObject.transform.translation.z = -2.5f;
    KeyboardMouvementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    
    int i = 0;
    while (!lveWindow.shouldClose())
    {

      glfwPollEvents();

      auto newTime = std::chrono::high_resolution_clock::now();
      float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
      currentTime = newTime;
      //display frame time and fps
      
      cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewerObject);
      camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

      float aspect = lveRenderer.getAspectRatio();
      // camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
      camera.setPerspectiveProjection(glm::radians(80.f), aspect, 0.1f, 100.f);
      if (auto commandBuffer = lveRenderer.beginFrame())
      {
        
        int frameIndex = lveRenderer.getFrameIndex();
        i = i + 1;
        std::cout << "\033[4A";
        std::cout << "Frame time: " << frameTime << " seconds" << std::endl;
        std::cout << "frame per second :" << 1.f/frameTime << std::endl;
        FrameInfo frameInfo{
            frameIndex, frameTime, commandBuffer, camera, globalDescriptorSets[frameIndex], gameObjects, lveRenderer.getCurrentComputeCommandBuffer()};

        // update
        GlobalUbo ubo{};

        ubo.projection = camera.getProjection();
        ubo.view = camera.getView();
        ubo.inverseView = camera.getInverseView();
        pointLightSystem.update(frameInfo, ubo);
        uboBuffers[frameIndex]->writeToBuffer(&ubo);
        uboBuffers[frameIndex]->flush();

        // render
        lveRenderer.beginSwapChainRenderPass(commandBuffer);

        // order here
        simpleRenderSystem.renderGameObjects(frameInfo);
        pointLightSystem.render(frameInfo);

        lveRenderer.endSwapChainRenderPass(commandBuffer);
        lveRenderer.endFrame();
        lveRenderer.renderPostProssessingEffects(frameInfo);
        lveRenderer.presentFrame();
      }
    }

    vkDeviceWaitIdle(lveDevice.device());
  }

  void FirstApp::loadGameObjects()
  {

    std::shared_ptr<LveModel> lveModel = LveModel::createModelFromFile(lveDevice, "models/smooth_vase.obj");
    auto flatVase = LveGameObject::createGameObject();
    flatVase.model = lveModel;
    flatVase.transform.translation = {-.7f, -0.1f, 0.f};
    flatVase.transform.scale = {1.2f, 1.2f, 1.2f};
    flatVase.transform.rotation = {0.f, glm::radians(90.f), 0.f};
    gameObjects.emplace(flatVase.getId(), std::move(flatVase));

    lveModel = LveModel::createModelFromFile(lveDevice, "models/Rubber Duck jaune.obj");
    std::shared_ptr<LveTexture> lveTexture = std::make_unique<LveTexture>(lveDevice, "textures/ayaka.jpg", false);
    auto smoothVase = LveGameObject::createGameObject();
    (*lveModel).createDescriptorSet(lveDevice, lveTexture.get(), nullptr);
    smoothVase.texture = lveTexture;
    smoothVase.model = lveModel;
    smoothVase.transform.translation = {.5f, 0.5f, 0.0f};
    smoothVase.transform.scale = {0.3f, 0.3f, 0.3f};
    smoothVase.transform.rotation = {0.f, glm::radians(180.f), glm::radians(180.f)};
    gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

    

    lveModel = LveModel::createModelFromFile(lveDevice, "models/quad.obj");
    lveTexture = std::make_unique<LveTexture>(lveDevice, "textures/texture.jpg", false);
    auto floor = LveGameObject::createGameObject();
    (*lveModel).createDescriptorSet(lveDevice, lveTexture.get(), nullptr);
    floor.model = lveModel;
    floor.texture = lveTexture;
    floor.transform.translation = {0.f, .5f, 0.f};
    floor.transform.scale = {3.f, 1.f, 3.f};
    gameObjects.emplace(floor.getId(), std::move(floor));

    // lveModel = LveModel::createModelFromFile(lveDevice, "models/quad.obj");
    // auto test = LveGameObject::createGameObject();
    // (*lveModel).createDescriptorSet(lveDevice, texturedst.get(), nullptr);
    // test.model = lveModel;
    // test.texture = texturedst;
    // test.transform.translation = {0.f, -1.5f, 0.f};
    // test.transform.scale = {3.f, 1.f, 3.f};
    // test.transform.rotation = {0.f, glm::radians(180.f), glm::radians(180.f)};
    // gameObjects.emplace(test.getId(), std::move(test));

    std::vector<glm::vec3> lightColors{
        {1.f, .1f, .1f},
        {.1f, .1f, 1.f},
        {.1f, 1.f, .1f},
        {1.f, 1.f, .1f},
        {.1f, 1.f, 1.f},
        {1.f, 1.f, 1.f} //
    };
    std::cout << "C'EST ptet ici" << std::endl;
    for (int i = 0; i < lightColors.size(); i++)
    {
      auto pointLight = LveGameObject::makePointLight(0.6f);
      // pointLight.color = lightColors[i];
      pointLight.color = {1.f, 1.f, 1.f};
      auto rotateLight = glm::rotate(
          glm::mat4(1.f),
          (i * glm::two_pi<float>()) / lightColors.size(),
          {0.f, -1.f, 0.f});
      pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
      gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }

    // using pointlight again invalid
  }
} // namespace lve