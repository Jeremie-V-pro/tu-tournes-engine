#include "first_app.hpp"

#include <vulkan/vulkan_core.h>

#include <cmath>
#include <cstdint>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <iostream>
#include <utility>
#include <vector>

#include "keyboard_mouvement_controller.hpp"
#include "lve_buffer.hpp"
#include "lve_camera.hpp"
#include "lve_descriptor.hpp"
#include "lve_device.hpp"
#include "lve_game_object.hpp"
#include "lve_swap_chain.hpp"
#include "systems/computesSystems/shaderToySystem.hpp"
#include "systems/computesSystems/waveGenerationSystem.hpp"
#include "systems/graphicsSystems/point_light_system.hpp"
#include "systems/graphicsSystems/simple_render_system.hpp"
#include "systems/graphicsSystems/sun_system.hpp"
#include "systems/graphicsSystems/water_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <array>
#include <chrono>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>
#include <stdexcept>

#include "lve_texture.hpp"

namespace lve {

FirstApp::FirstApp() {
    globalPool = LveDescriptorPool::Builder(lveDevice)
                     .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                     .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                     .build();

    // Set default descriptor layout
    std::shared_ptr<LveDescriptorSetLayout::Builder> setLayoutBuilder =
        std::make_shared<LveDescriptorSetLayout::Builder>(lveDevice);
    setLayoutBuilder->addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    LveDescriptorSetLayout::defaultTextureSetLayout = setLayoutBuilder->build();

    float boundary1 = 2 * M_PI / 17.f * 6.f;
    float boundary2 = 2 * M_PI / 5.f * 6.f;
    waveGen1 = std::make_shared<WaveGen>(lveDevice, 250, 0.0001f, boundary1);
    waveGen2 = std::make_shared<WaveGen>(lveDevice, 17, boundary1, boundary2);
    waveGen3 = std::make_shared<WaveGen>(lveDevice, 5, boundary2, 9999.f);

    display = waveGen2->getDisplacement();
    derivatives = waveGen2->getDerivatives();
    turbu = waveGen2->getTurbulence();

    loadGameObjects();
}

FirstApp::~FirstApp() {}

void FirstApp::run() {
    std::cout << "FirstApp::run()" << std::endl;

    // Création des uniform buffer pour chaques frames
    std::vector<std::unique_ptr<LveBuffer>> uboBuffers(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] = std::make_unique<LveBuffer>(lveDevice, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        uboBuffers[i]->map();
    }

    // Création du layout pour les uniform buffer
    auto globalSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
                               .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                           VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT)
                               .build();

    // Création des descriptor sets pour les uniform buffer
    std::vector<VkDescriptorSet> globalDescriptorSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        LveDescriptorWriter(*globalSetLayout, *globalPool).writeBuffer(0, &bufferInfo).build(globalDescriptorSets[i]);
    }

    // initialisation du system de rendu des luimères
    PointLightSystem pointLightSystem{lveDevice, lveRenderer.getSwapChainRenderPass(),
                                      globalSetLayout->getDescriptorSetLayout()};

    WaterSystem WaterRenderSystem{lveDevice,
                                  lveRenderer.getSwapChainRenderPass(),
                                  globalSetLayout->getDescriptorSetLayout(),
                                  waveGen1->getAllDisplacement(),
                                  waveGen1->getAllDerivatives(),
                                  waveGen1->getAllTurbulence(),
                                  waveGen2->getAllDisplacement(),
                                  waveGen2->getAllDerivatives(),
                                  waveGen2->getAllTurbulence(),
                                  waveGen3->getAllDisplacement(),
                                  waveGen3->getAllDerivatives(),
                                  waveGen3->getAllTurbulence()};

    // initialisation du system de rendu simple
    SimpleRenderSystem simpleRenderSystem{lveDevice,
                                          lveRenderer.getSwapChainRenderPass(),
                                          globalSetLayout->getDescriptorSetLayout(),
                                          LveDescriptorSetLayout::defaultTextureSetLayout->getDescriptorSetLayout(),
                                          WaterRenderSystem.getWaterTextureSetLayout(),
                                          WaterRenderSystem.getDescriptorSets()};

    SunSystem sunSystem{lveDevice, lveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout(),
                        sun};

    std::shared_ptr<ShaderToySystem> testToyShader = std::make_shared<ShaderToySystem>(
        lveDevice, globalSetLayout->getDescriptorSetLayout(),
        LveDescriptorSetLayout::defaultPostProcessingTextureSetLayout->getDescriptorSetLayout(),
        LveDescriptorSetLayout::depthTextureSetLayout->getDescriptorSetLayout());

    lveRenderer.addPostProcessingEffect(testToyShader);
    lveRenderer.addPreProcessingEffect(waveGen1);
    lveRenderer.addPreProcessingEffect(waveGen2);
    lveRenderer.addPreProcessingEffect(waveGen3);
    LveCamera camera{};
    // camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.5, 0.f, 1.f));
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

    auto viewerObject = LveGameObject::createGameObject();
    viewerObject.transform.translation.z = -2.5f;
    KeyboardMouvementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    int i = 0;
    while (!lveWindow.shouldClose()) {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;
        // display frame time and fps

        cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewerObject);

        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
        // change horizontal position of the water
        gameObjects.at(waterId).transform.translation.z = viewerObject.transform.translation.z * 2.f;
        gameObjects.at(waterId).transform.translation.x = viewerObject.transform.translation.x * 2.f;
        glm::vec3 sunDirection = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
        float distanceFromCamera = 50.0f;
        glm::vec3 sunPosition = viewerObject.transform.translation + distanceFromCamera * sunDirection;
        sun->transform.translation = sunPosition;

        // Déplacer le point par la distance spécifiée
        float distance = 2.0f;
        glm::vec3 translationVector(0.0f, distance, 0.0f);

        float aspect = lveRenderer.getAspectRatio();
        camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
        camera.setPerspectiveProjection(glm::radians(80.f), aspect, 0.1f, 100.f);
        if (lveRenderer.startRendering(syncObjects)) {
            VkCommandBuffer commandBuffer = lveRenderer.beginFrame(syncObjects);
            int frameIndex = lveRenderer.getFrameIndex();
            int swapChainImageIndex = lveRenderer.getSwapchainFrameIndex();
            i = i + 1;

            std::cout << "Frame time: " << frameTime << " seconds" << std::endl;
            std::cout << "frame per second :" << 1.f / frameTime << std::endl;
            std::cout << "\033[2A";
            FrameInfo frameInfo{frameIndex,
                                swapChainImageIndex,
                                frameTime,
                                commandBuffer,
                                lveRenderer.getPreProcessingCommandBuffer(),
                                lveRenderer.getPostProcessingCommandBuffer(),
                                camera,
                                globalDescriptorSets[frameIndex],
                                gameObjects};

            lveRenderer.executePreProssessingEffects(frameInfo, syncObjects);
            // update
            GlobalUbo ubo{};

            ubo.projection = camera.getProjection();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();
            ubo.sunDirection = glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f);
            pointLightSystem.update(frameInfo, ubo);
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            // render
            lveRenderer.beginSwapChainRenderPass(commandBuffer);

            // order here
            simpleRenderSystem.renderGameObjects(frameInfo);
            WaterRenderSystem.renderGameObjects(frameInfo);
            pointLightSystem.render(frameInfo);
            sunSystem.render(frameInfo);
            lveRenderer.endSwapChainRenderPass(commandBuffer);
            lveRenderer.endFrame(syncObjects);
            lveRenderer.renderPostProssessingEffects(frameInfo, syncObjects);
            lveRenderer.presentFrame(syncObjects);
        }
    }

    vkDeviceWaitIdle(lveDevice.device());
}

void FirstApp::loadGameObjects() {
    // std::shared_ptr<LveModel> lveModel = LveModel::createModelFromFile(lveDevice, "models/smooth_vase.obj");
    // auto flatVase = LveGameObject::createGameObject();
    // flatVase.model = lveModel;
    // flatVase.transform.translation = {-.7f, -0.1f, 0.f};
    // flatVase.transform.scale = {1.2f, 1.2f, 1.2f};
    // flatVase.transform.rotation = {0.f, glm::radians(90.f), 0.f};
    // gameObjects.emplace(flatVase.getId(), std::move(flatVase));

    std::shared_ptr<LveModel> lveModel = LveModel::createModelFromFile(lveDevice, "models/Rubber Duck jaune.obj");
    std::shared_ptr<LveTexture> lveTexture = std::make_unique<LveTexture>(lveDevice, "textures/Rubber_Duck.png", false);
    auto coin = LveGameObject::createGameObject();
    (*lveModel).createDescriptorSet(lveDevice, lveTexture.get(), nullptr);
    coin.texture = lveTexture;
    coin.model = lveModel;
    coin.transform.translation = {.5f, 0.35f, 0.0f};
    coin.transform.scale = {0.15f, 0.15f, 0.15f};
    coin.transform.rotation = {0.f, glm::radians(180.f), glm::radians(180.f)};
    gameObjects.emplace(coin.getId(), std::move(coin));

    // std::shared_ptr<LveModel> lveModel = LveModel::createModelFromFile(lveDevice, "models/quad.obj");
    // auto floor = LveGameObject::createGameObject();
    // (*lveModel).createDescriptorSet(lveDevice, display.get(), nullptr);
    // floor.model = lveModel;
    // floor.texture = display;
    // floor.transform.translation = {0.f, .5f, -8.f};
    // floor.transform.scale = {3.f, 1.f, 3.f};
    // gameObjects.emplace(floor.getId(), std::move(floor));

    // lveModel = LveModel::createModelFromFile(lveDevice, "models/quad.obj");
    // floor = LveGameObject::createGameObject();
    // (*lveModel).createDescriptorSet(lveDevice, derivatives.get(), nullptr);
    // floor.model = lveModel;
    // floor.texture = derivatives;
    // floor.transform.translation = {8.f, .5f, 0.f};
    // floor.transform.scale = {3.f, 1.f, 3.f};
    // gameObjects.emplace(floor.getId(), std::move(floor));

    // lveModel = LveModel::createModelFromFile(lveDevice, "models/quad.obj");
    // floor = LveGameObject::createGameObject();
    // (*lveModel).createDescriptorSet(lveDevice, turbu.get(), nullptr);
    // floor.model = lveModel;
    // floor.texture = turbu;
    // floor.transform.translation = {0.f, .5f, 8.f};
    // floor.transform.scale = {3.f, 1.f, 3.f};
    // gameObjects.emplace(floor.getId(), std::move(floor));

    lveModel = LveModel::createModelFromFile(lveDevice, "models/ocean.obj");
    auto floor = LveGameObject::createGameObject();
    floor.model = lveModel;
    floor.water = std::make_unique<Water>();
    floor.transform.translation = {0.f, .5f, 0.f};
    floor.transform.scale = {1.f, 1.f, 1.f};
    floor.transform.rotation = {0.f, 0.f, 0.f};
    waterId = floor.getId();
    gameObjects.emplace(floor.getId(), std::move(floor));

    sun = std::make_shared<LveGameObject>(LveGameObject::createGameObject());

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
        {1.f, .1f, .1f}, {.1f, .1f, 1.f}, {.1f, 1.f, .1f}, {1.f, 1.f, .1f}, {.1f, 1.f, 1.f}, {1.f, 1.f, 1.f}  //
    };

    for (int i = 0; i < lightColors.size(); i++) {
        auto pointLight = LveGameObject::makePointLight(0.6f);
        pointLight.color = lightColors[i];
        // pointLight.color = {1.f, 1.f, 1.f};
        auto rotateLight =
            glm::rotate(glm::mat4(1.f), (i * glm::two_pi<float>()) / lightColors.size(), {0.f, -1.f, 0.f});
        pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }

    // using pointlight again invalid
}
}  // namespace lve