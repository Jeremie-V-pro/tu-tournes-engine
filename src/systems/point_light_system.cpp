#include "point_light_system.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include <glm/fwd.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <map>
#include <stdexcept>

namespace lve {

struct PointLightPushConstants{
  glm::vec4 position{};
  glm::vec4 color{};
  float radius;
};


PointLightSystem::PointLightSystem(LveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : lveDevice{device} {
  createPipelineLayout(globalSetLayout);
  createPipeLine(renderPass);
  time = 0.0f;
}


PointLightSystem::~PointLightSystem() {
  vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
}


void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PointLightPushConstants);

  std::vector<VkDescriptorSetLayout> descriptorSetLayout{globalSetLayout};
  
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayout.size());
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayout.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}


void PointLightSystem::createPipeLine(VkRenderPass renderPass) {
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before pipeline layout");

  PipelineConfigInfo pipelineConfig{};
  LvePipeline::defaultPipeLineConfigInfo(pipelineConfig);
  LvePipeline::enableAlphaBlending(pipelineConfig);
  pipelineConfig.attributeDescriptions.clear();
  pipelineConfig.bindingDescriptions.clear();
  pipelineConfig.renderPass = renderPass;
  pipelineConfig.pipelineLayout = pipelineLayout;
  lvePipeline = std::make_unique<LvePipeline>(
      lveDevice,
      "shaders/point_light.vert.spv",
      "shaders/point_light.frag.spv",
      pipelineConfig);
}


void PointLightSystem::update(FrameInfo &frameInfo, GlobalUbo &ubo) {
  int lightIndex = 0;
  
  auto rotateLight = glm::rotate(glm::mat4(1.f), frameInfo.frameTime * 1, {0.f, -1.f, 0.f});
  float speed = 1.0f;
  
  for(auto &kv: frameInfo.gameObjects){
    auto& obj = kv.second;
    if(obj.pointLight == nullptr) continue;
    

    float r = 0.5f + 0.5f * std::sin(speed * time + lightIndex);
    float g = 0.5f + 0.5f * std::sin(speed * time + lightIndex + 2.0f * M_PI / 3.0f);
    float b = 0.5f + 0.5f * std::sin(speed * time + lightIndex+ 4.0f * M_PI / 3.0f);
    glm::vec3 rgb = glm::vec3(r, g, b);
    obj.color = rgb;
    
    assert(lightIndex < MAX_LIGHTS && "Point light exceed maximum specified");
    
    // update light position
    obj.transform.translation = glm::vec3 (rotateLight * glm::vec4 (obj.transform.translation, 1.f) );
    
    // copy light position
    ubo.pointLights[lightIndex].position = glm::vec4 (obj.transform.translation, 1.f);
    ubo.pointLights[lightIndex].color = glm::vec4 (obj.color, obj.pointLight->lightIntensity);
    lightIndex += 1;
  }
  ubo.numLights = lightIndex;
  time+=0.01;
}


void PointLightSystem::render(FrameInfo &frameInfo) {

  std::map<float, LveGameObject::id_t> sorted;
  for(auto &kv: frameInfo.gameObjects){
    auto& obj = kv.second;
    if(obj.pointLight == nullptr) continue;
    auto offset = frameInfo.camera.getPosition() - obj.transform.translation;
    float disSquared = glm::dot(offset, offset);
    sorted[disSquared] = obj.getId();
  }

  lvePipeline->bind(frameInfo.commandBuffer);
  
  vkCmdBindDescriptorSets(
  frameInfo.commandBuffer,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipelineLayout,
    0,1,
  &frameInfo.globalDescriptorSet,
    0, nullptr);
  
  
  for(auto it = sorted.rbegin(); it != sorted.rend(); ++it){
    auto& obj = frameInfo.gameObjects.at(it->second);

    PointLightPushConstants push{};
    push.position = glm::vec4 (obj.transform.translation, 1.f);
    push.color = glm::vec4 (obj.color, obj.pointLight->lightIntensity);
    push.radius = obj.transform.scale.x;
    vkCmdPushConstants(
        frameInfo.commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PointLightPushConstants),
        &push
        );
   vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0 ,0); 
    
  }
}

} // namespace lve