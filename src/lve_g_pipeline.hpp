#pragma once
#include "lve_device.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace lve{



struct PipelineConfigInfo {
  PipelineConfigInfo() = default;
  PipelineConfigInfo(const PipelineConfigInfo&) = delete;
  PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

  std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
  VkPipelineViewportStateCreateInfo viewportInfo;
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
  VkPipelineRasterizationStateCreateInfo rasterizationInfo;
  VkPipelineMultisampleStateCreateInfo multisampleInfo;
  VkPipelineColorBlendAttachmentState colorBlendAttachment;
  VkPipelineColorBlendStateCreateInfo colorBlendInfo;
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
  std::vector<VkDynamicState> dynamicStateEnables;
  VkPipelineDynamicStateCreateInfo dynamicStateInfo;
  VkPipelineLayout pipelineLayout = nullptr;
  VkRenderPass renderPass = nullptr;
  uint32_t subpass = 0;
};

class LveGPipeline
{
public:
  LveGPipeline(LveDevice &device,
              const std::string& vertFilepath,
              const std::string& fragFilepath,
              const PipelineConfigInfo& configInfo);
  ~LveGPipeline();

  LveGPipeline(const LveGPipeline&) = delete;
  LveGPipeline& operator=(const LveGPipeline&) = delete;


  void bind(VkCommandBuffer commandBuffer);

  static void defaultPipeLineConfigInfo(PipelineConfigInfo& configInfo);
  static void enableAlphaBlending(PipelineConfigInfo& configInfo);

private:
  static std::vector<char> readFile(const std::string& filepath);
  
  

  void creatGraphicsPipeline(const std::string& vertFilepath,
                             const std::string& fragFilepath,
                             const PipelineConfigInfo& configInfo);
  void createShaderModule(const std::vector<char>& code,VkShaderModule* shaderModule);

  LveDevice& lveDevice;
  VkPipeline graphicsPipeLine;
  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;
};

}