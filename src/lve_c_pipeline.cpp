#include "lve_c_pipeline.hpp"
#include "lve_model.hpp"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <cassert>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace lve
{
  LveCPipeline::LveCPipeline(LveDevice &device, const std::string &computeFilepath, const ComputePipelineConfigInfo &configInfo) : lveDevice{device}
  {
    createComputePipeline(computeFilepath, configInfo);
  }

  LveCPipeline::~LveCPipeline()
  {
    vkDestroyShaderModule(lveDevice.device(), computeShaderModule, nullptr);
    vkDestroyPipeline(lveDevice.device(), computePipeLine, nullptr);
  }

  std::vector<char> LveCPipeline::readFile(const std::string &filepath)
  {

    std::string enginePath = ENGINE_DIR + filepath;

    std::ifstream file{enginePath, std::ios::ate | std::ios::binary};

    if (!file.is_open())
    {
      throw std::runtime_error("failed to open file : " + enginePath);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
  }



  void LveCPipeline::createComputePipeline(const std::string &computeFilepath, const ComputePipelineConfigInfo &configInfo)
  {
    assert(configInfo.computePipelineLayout != VK_NULL_HANDLE && "Cannot create compute pipeline:: no pipelineLayout provided in configInfo");
    auto compCode = readFile(computeFilepath);

    
    createComputeShaderModule(compCode, &computeShaderModule);
    
    VkPipelineShaderStageCreateInfo shaderStages{};

    shaderStages.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStages.module = computeShaderModule;
    shaderStages.pName = "main";


    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = shaderStages;
    pipelineInfo.layout = configInfo.computePipelineLayout;

    VkResult test =  vkCreateComputePipelines(lveDevice.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeLine);
    if (test != VK_SUCCESS)
    {
      
      throw std::runtime_error("failed to create graphic pipeline");
    }
    else
    {
      std::cout << "Pipeline created" << std::endl;
    }
  }

  void LveCPipeline::createComputeShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule)
  {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    if (vkCreateShaderModule(lveDevice.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create shader module");
    }
  }

  void LveCPipeline::bind(VkCommandBuffer commandBuffer)
  {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeLine);
  }

  void LveCPipeline::defaultPipeLineConfigInfo(ComputePipelineConfigInfo &configInfo)
  {

  }

}