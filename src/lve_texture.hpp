#pragma once
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "lve_device.hpp"

namespace lve {
class LveTexture {
   public:
    LveTexture(LveDevice& device, const std::string& filepath, bool isComputeTexture);
    LveTexture(LveDevice& device, int width, int height);

    LveTexture(LveDevice& device, int width, int height, void* image, int numberOfChannels, VkFormat textureFormat);
    ~LveTexture();

    VkSampler getSampler() const { return sampler; }
    VkImageView getImageView() const { return imageView; }
    VkImageLayout getImageLayout() const { return imageLayout; }
    VkImage getTextureImage() const { return textureImage; }

    void objectTextureConstructor(const std::string& filepath);

    void computeTextureConstructor(const std::string& filepath);

    void postprocessingTextureConstructor(int width, int height);

    void cpuTextureConstructor(int width, int height, void* image, int numberOfChannels, VkFormat textureFormat);

    static void copyTexture(VkCommandBuffer commandBuffer, std::shared_ptr<LveTexture> textureFromCopy,
                            std::shared_ptr<LveTexture> textureToCopy);

    int width;
    int height;

   private:
    LveDevice& lveDevice;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView imageView;
    VkSampler sampler;
    VkFormat imageFormat;
    VkImageLayout imageLayout;

    void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
};
}  // namespace lve