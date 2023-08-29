#pragma once
#include "lve_device.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace lve{



struct ComputePipelineConfigInfo {
  ComputePipelineConfigInfo() = default;
  ComputePipelineConfigInfo(const ComputePipelineConfigInfo&) = delete;
  ComputePipelineConfigInfo& operator=(const ComputePipelineConfigInfo&) = delete;

  VkPipelineShaderStageCreateInfo computeShaderStageInfo;
  VkPipelineLayout computePipelineLayout = nullptr;

};

class LveCPipeline
{
public:
  LveCPipeline(LveDevice &device,
              const std::string& computeFilepath,
              const ComputePipelineConfigInfo& configInfo);
  ~LveCPipeline();

  LveCPipeline(const LveCPipeline&) = delete;
  LveCPipeline& operator=(const LveCPipeline&) = delete;


  void bind(VkCommandBuffer commandBuffer);

  static void defaultPipeLineConfigInfo(ComputePipelineConfigInfo& configInfo);

private:
  static std::vector<char> readFile(const std::string& filepath);
  
  

  void createComputePipeline(const std::string& computeFilepath,
                             const ComputePipelineConfigInfo& configInfo);
  void createComputeShaderModule(const std::vector<char>& code,VkShaderModule* shaderModule);

  LveDevice& lveDevice;
  VkPipeline computePipeLine;
  VkShaderModule computeShaderModule;
};

}