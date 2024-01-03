#include <vulkan/vulkan_core.h>

#include <cstdint>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cassert>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

#include "lve_buffer.hpp"
#include "lve_texture.hpp"

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace lve {

LveTexture::LveTexture(LveDevice &device, const std::string &filepath, bool isComputeTexture)
    : lveDevice{device}, width{0}, height{0} {
    if (isComputeTexture) {
        computeTextureConstructor(filepath);
    } else {
        objectTextureConstructor(filepath);
    }
}

LveTexture::LveTexture(LveDevice &device, int width, int height) : lveDevice{device}, width{width}, height{height} {
    postprocessingTextureConstructor(width, height);
}

LveTexture::LveTexture(LveDevice &device, int width, int height, void *image, int numberOfChannels,
                       VkFormat textureFormat)
    : lveDevice{device}, width(width), height(height) {
    cpuTextureConstructor(width, height, image, numberOfChannels, textureFormat);
}

void LveTexture::postprocessingTextureConstructor(int width, int height) {
    LveBuffer stagingBuffer{lveDevice, 4, static_cast<u_int32_t>(width * height), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

    imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = imageFormat;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    lveDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

    transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    // lveDevice.copyBufferToImage(stagingBuffer.getBuffer(), textureImage,
    // static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1,
    // imageLayout);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;  // VK_FILTER_LINEAR
    samplerInfo.minFilter = VK_FILTER_NEAREST;  // VK_FILTER_LINEAR
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;  // VK_COMPARE_OP_NEVER
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.maxAnisotropy = 4.0f;
    samplerInfo.anisotropyEnable = VK_TRUE;                      // VK_FALSE
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;  // VK_BORDER_COLOR_INT_OPAQUE_BLACK

    vkCreateSampler(lveDevice.device(), &samplerInfo, nullptr, &sampler);

    VkImageViewCreateInfo imageViewInfo{};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  // VK_IMAGE_VIEW_TYPE_2D
    imageViewInfo.format = imageFormat;
    imageViewInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
                                VK_COMPONENT_SWIZZLE_A};                    // VK_COMPONENT_SWIZZLE_IDENTITY
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // VK_IMAGE_ASPECT_COLOR_BIT
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.image = textureImage;

    vkCreateImageView(lveDevice.device(), &imageViewInfo, nullptr, &imageView);
}

void LveTexture::objectTextureConstructor(const std::string &filepath) {
    int width, height, channels;
    int byPerPixel;
    stbi_set_flip_vertically_on_load(true);
    stbi_uc *pixels = stbi_load((ENGINE_DIR + filepath).c_str(), &width, &height, &byPerPixel, 4);
    LveBuffer stagingBuffer{lveDevice, 4, static_cast<u_int32_t>(width * height), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    stagingBuffer.map();
    stagingBuffer.writeToBuffer(pixels);
    imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = imageFormat;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    lveDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

    transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    lveDevice.copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(width),
                                static_cast<uint32_t>(height), 1);

    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;  // VK_FILTER_LINEAR
    samplerInfo.minFilter = VK_FILTER_LINEAR;  // VK_FILTER_LINEAR
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;  // VK_COMPARE_OP_NEVER
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.maxAnisotropy = 4.0f;
    samplerInfo.anisotropyEnable = VK_TRUE;                      // VK_FALSE
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;  // VK_BORDER_COLOR_INT_OPAQUE_BLACK

    vkCreateSampler(lveDevice.device(), &samplerInfo, nullptr, &sampler);

    VkImageViewCreateInfo imageViewInfo{};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  // VK_IMAGE_VIEW_TYPE_2D
    imageViewInfo.format = imageFormat;
    imageViewInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
                                VK_COMPONENT_SWIZZLE_A};                    // VK_COMPONENT_SWIZZLE_IDENTITY
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // VK_IMAGE_ASPECT_COLOR_BIT
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.image = textureImage;

    vkCreateImageView(lveDevice.device(), &imageViewInfo, nullptr, &imageView);

    stbi_image_free(pixels);
}

void LveTexture::computeTextureConstructor(const std::string &filepath) {
    int width, height, channels;
    int byPerPixel;

    stbi_uc *pixels = stbi_load((ENGINE_DIR + filepath).c_str(), &width, &height, &byPerPixel, 4);
    LveBuffer stagingBuffer{lveDevice, 4, static_cast<u_int32_t>(width * height), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    stagingBuffer.map();
    stagingBuffer.writeToBuffer(pixels);
    imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = imageFormat;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    lveDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

    transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    lveDevice.copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(width),
                                static_cast<uint32_t>(height), 1, imageLayout);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;  // VK_FILTER_LINEAR
    samplerInfo.minFilter = VK_FILTER_NEAREST;  // VK_FILTER_LINEAR
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;  // VK_COMPARE_OP_NEVER
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.maxAnisotropy = 4.0f;
    samplerInfo.anisotropyEnable = VK_TRUE;                      // VK_FALSE
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;  // VK_BORDER_COLOR_INT_OPAQUE_BLACK

    vkCreateSampler(lveDevice.device(), &samplerInfo, nullptr, &sampler);

    VkImageViewCreateInfo imageViewInfo{};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  // VK_IMAGE_VIEW_TYPE_2D
    imageViewInfo.format = imageFormat;
    imageViewInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
                                VK_COMPONENT_SWIZZLE_A};                    // VK_COMPONENT_SWIZZLE_IDENTITY
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // VK_IMAGE_ASPECT_COLOR_BIT
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.image = textureImage;

    vkCreateImageView(lveDevice.device(), &imageViewInfo, nullptr, &imageView);

    stbi_image_free(pixels);
}

void LveTexture::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = lveDevice.beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;                          // VK_IMAGE_LAYOUT_UNDEFINED
    barrier.newLayout = newLayout;                          // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // VK_QUEUE_FAMILY_IGNORED
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // VK_QUEUE_FAMILY_IGNORED
    barrier.image = textureImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // VK_IMAGE_ASPECT_COLOR_BIT
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;                             // VK_ACCESS_TRANSFER_WRITE_BIT
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;  // 0

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;    // VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;  // VK_PIPELINE_STAGE_TRANSFER_BIT
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;  // 0
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;     // 0

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;              // VK_PIPELINE_STAGE_TRANSFER_BIT
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;  // VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;  // 0
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;     // 0

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;  // VK_PIPELINE_STAGE_TRANSFER_BIT
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;  // VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    lveDevice.endSingleTimeCommands(commandBuffer);
}

void LveTexture::cpuTextureConstructor(int width, int height, void *image, int numberOfChannels,
                                       VkFormat textureFormat) {
    if (textureFormat == VK_FORMAT_R32G32_SFLOAT || textureFormat == VK_FORMAT_R32G32B32A32_SFLOAT)
        numberOfChannels = numberOfChannels * 4;
    LveBuffer stagingBuffer{lveDevice, (unsigned long)(unsigned int)numberOfChannels,
                            static_cast<u_int32_t>(width * height), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    stagingBuffer.map();
    stagingBuffer.writeToBuffer(image);
    imageFormat = textureFormat;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = imageFormat;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    lveDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

    transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    lveDevice.copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(width),
                                static_cast<uint32_t>(height), 1);

    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

    imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;  // VK_FILTER_LINEAR
    samplerInfo.minFilter = VK_FILTER_LINEAR;  // VK_FILTER_LINEAR
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;  // VK_COMPARE_OP_NEVER
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.maxAnisotropy = 8.0f;
    samplerInfo.anisotropyEnable = VK_TRUE;                      // VK_FALSE
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;  // VK_BORDER_COLOR_INT_OPAQUE_BLACK

    vkCreateSampler(lveDevice.device(), &samplerInfo, nullptr, &sampler);

    VkImageViewCreateInfo imageViewInfo{};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  // VK_IMAGE_VIEW_TYPE_2D
    imageViewInfo.format = imageFormat;
    imageViewInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
                                VK_COMPONENT_SWIZZLE_A};                    // VK_COMPONENT_SWIZZLE_IDENTITY
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // VK_IMAGE_ASPECT_COLOR_BIT
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.image = textureImage;
    vkCreateImageView(lveDevice.device(), &imageViewInfo, nullptr, &imageView);
}

void LveTexture::copyTexture(VkCommandBuffer commandBuffer, std::shared_ptr<LveTexture> textureFromCopy,
                             std::shared_ptr<LveTexture> textureToCopy) {
    VkImageMemoryBarrier transferFromImageBarrier{};
    transferFromImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferFromImageBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    transferFromImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferFromImageBarrier.oldLayout = textureFromCopy->getImageLayout();
    transferFromImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferFromImageBarrier.image = textureFromCopy->getTextureImage();
    transferFromImageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkImageMemoryBarrier transferToImageBarrier{};
    transferToImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferToImageBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    transferToImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferToImageBarrier.oldLayout = textureToCopy->getImageLayout();
    transferToImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferToImageBarrier.image = textureToCopy->getTextureImage();
    transferToImageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &transferFromImageBarrier);

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &transferToImageBarrier);

    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = textureFromCopy->width;
    imageCopyRegion.extent.height = textureFromCopy->height;
    imageCopyRegion.extent.depth = 1;

    // Issue the copy command
    vkCmdCopyImage(commandBuffer, textureFromCopy->getTextureImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   textureToCopy->getTextureImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

    VkImageMemoryBarrier transferFromBackImageBarrier{};
    transferFromBackImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferFromBackImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferFromBackImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    transferFromBackImageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferFromBackImageBarrier.newLayout = textureFromCopy->getImageLayout();
    transferFromBackImageBarrier.image = textureFromCopy->getTextureImage();
    transferFromBackImageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &transferFromBackImageBarrier);

    VkImageMemoryBarrier transferToBackImageBarrier{};
    transferToBackImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferToBackImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferToBackImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    transferToBackImageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferToBackImageBarrier.newLayout = textureToCopy->getImageLayout();
    transferToBackImageBarrier.image = textureToCopy->getTextureImage();
    transferToBackImageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &transferToBackImageBarrier);
}

LveTexture::~LveTexture() {
    vkDestroyImage(lveDevice.device(), textureImage, nullptr);
    vkFreeMemory(lveDevice.device(), textureImageMemory, nullptr);
    vkDestroyImageView(lveDevice.device(), imageView, nullptr);
    vkDestroySampler(lveDevice.device(), sampler, nullptr);
}

}  // namespace lve