#pragma once

#include "lve_buffer.hpp"
#include "lve_device.hpp"
#include "lve_descriptor.hpp"

#include <memory>
#include <vector>


namespace lve{
class LveTexture{
  public:

    LveTexture(LveDevice& device, const std::string& filepath, bool isComputeTexture);
    LveTexture(LveDevice& device, int width, int height);
    ~LveTexture();

    LveTexture(const LveTexture &) = delete;
    LveTexture &operator=(const LveTexture &) = delete;
    LveTexture(LveTexture &&) = delete;
    LveTexture &operator=(LveTexture &&) = delete;

   

    VkSampler getSampler() const { return sampler; }
    VkImageView getImageView() const { return imageView; }
    VkImageLayout getImageLayout() const { return imageLayout; }

    void objectTextureConstructor(const std::string& filepath);

    void computeTextureConstructor(const std::string& filepath);

    void postprocessingTextureConstructor(int width, int height);

    

    

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
}