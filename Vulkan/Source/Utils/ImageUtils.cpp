#include "stdafx.h"
#include "ImageUtils.h"

#include <gli/gli.hpp>
#include <gli/generate_mipmaps.hpp>

bool image_utils::CreateTextureImage(SImageParams& params)
{
    if (params.in_image_path.empty())
        return utils::FatalError(g_Engine->Hwnd(), "Failed to load texture image - given path was empty");

    // Load texture
    gli::texture2d tex_data(gli::load(params.in_image_path));

    if (tex_data.empty())
        return utils::FatalError(g_Engine->Hwnd(), "Failed to load texture image");

    // Store texture size
    auto tex_extent = tex_data.extent();
    VkDeviceSize imageSize = tex_data.size();

    // Store texture format
    params.out_texture_format = MapGliFormat2VkFormat(tex_data.format());

    // Prepare mip maps
    params.out_mip_levels = tex_data.levels();
    if (params.out_mip_levels == 1 && params.in_generate_mip_maps)
    {
        tex_data = gli::generate_mipmaps<gli::texture2d>(tex_data, gli::FILTER_LINEAR);
        params.out_mip_levels = tex_data.levels();
    }

    // Prepare tmp staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    g_Engine->Renderer()->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // Move texture data into buffer
    void* data;
    vkMapMemory(g_Engine->Renderer()->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, tex_data.data(), static_cast<size_t>(imageSize));
    vkUnmapMemory(g_Engine->Renderer()->GetDevice(), stagingBufferMemory);

    // Create Image
    CreateImage(tex_extent.x, tex_extent.y, params.out_mip_levels, params.out_texture_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, params.out_texture_image, params.out_texture_image_memory);
 
    // Change image layput
    TransitionImageLayout(params.out_texture_image, params.out_texture_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, params.out_mip_levels);
    
    // Prepare buffer copy regions for each mip level
    uint32_t offset = 0;
    std::vector<VkBufferImageCopy> bufferCopyRegions;
    for (uint32_t i = 0; i < params.out_mip_levels; i++)
    {
        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = i;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = (uint32_t)(tex_data[i].extent().x);
        bufferCopyRegion.imageExtent.height = (uint32_t)(tex_data[i].extent().y);
        bufferCopyRegion.imageExtent.depth = 1;
        bufferCopyRegion.bufferOffset = offset;

        bufferCopyRegions.push_back(bufferCopyRegion);
        offset += (uint32_t)(tex_data[i].size());
    }

    // Copy texture from staging buffer into newly created image
    VkCommandBuffer commandBuffer = g_Engine->Renderer()->BeginSingleTimeCommands();
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, params.out_texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)(bufferCopyRegions.size()), bufferCopyRegions.data());
    g_Engine->Renderer()->EndSingleTimeCommands(commandBuffer);

    // Release staging buffer and texture
    tex_data.clear();
    vkDestroyBuffer(g_Engine->Renderer()->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(g_Engine->Renderer()->GetDevice(), stagingBufferMemory, nullptr);

    return true;
}

bool image_utils::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (VKRESULT(vkCreateImage(g_Engine->Renderer()->GetDevice(), &imageInfo, nullptr, &image)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create image");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(g_Engine->Renderer()->GetDevice(), image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = g_Engine->Renderer()->FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (VKRESULT(vkAllocateMemory(g_Engine->Renderer()->GetDevice(), &allocInfo, nullptr, &imageMemory)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to allocate image memory");

    vkBindImageMemory(g_Engine->Renderer()->GetDevice(), image, imageMemory, 0);
}

void image_utils::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = g_Engine->Renderer()->BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (g_Engine->Renderer()->HasStencilComponent(format))
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        utils::FatalError(g_Engine->Hwnd(), "Unsupported layout transition");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    g_Engine->Renderer()->EndSingleTimeCommands(commandBuffer);
}

void image_utils::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t mip_level /*= 0*/)
{
    VkCommandBuffer commandBuffer = g_Engine->Renderer()->BeginSingleTimeCommands();

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mip_level;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent =
    {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    g_Engine->Renderer()->EndSingleTimeCommands(commandBuffer);
}

VkFormat image_utils::MapGliFormat2VkFormat(int gli_format_type)
{
    switch (gli_format_type)
    {
    case gli::texture::format_type::FORMAT_RGBA_BP_UNORM_BLOCK16:
        return VkFormat::VK_FORMAT_BC7_UNORM_BLOCK;
    case gli::texture::format_type::FORMAT_RGBA_DXT3_UNORM_BLOCK16:
        return VkFormat::VK_FORMAT_BC2_UNORM_BLOCK;
    case gli::texture::format_type::FORMAT_RG_ATI2N_UNORM_BLOCK16:
        return VkFormat::VK_FORMAT_BC5_UNORM_BLOCK;
    case gli::texture::format_type::FORMAT_R_ATI1N_UNORM_BLOCK8:
        return VkFormat::VK_FORMAT_BC4_UNORM_BLOCK;
    case gli::texture::format_type::FORMAT_RGBA_DXT5_UNORM_BLOCK16:
        return VkFormat::VK_FORMAT_BC3_UNORM_BLOCK;
    case gli::texture::format_type::FORMAT_RGBA_DXT1_UNORM_BLOCK8:
        return VkFormat::VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case gli::texture::format_type::FORMAT_BGRA8_UNORM_PACK8:
        return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
    }
    utils::FatalError(g_Engine->Hwnd(), "Unknown texture format");
    return VkFormat::VK_FORMAT_UNDEFINED;
}

VkFormat image_utils::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(g_Engine->Renderer()->GetPhysicalDevice(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    utils::FatalError(g_Engine->Hwnd(), "Failed to find supported format");
    return VkFormat();
}

VkImageView image_utils::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (VKRESULT(vkCreateImageView(g_Engine->Renderer()->GetDevice(), &viewInfo, nullptr, &imageView)))
    {
        utils::FatalError(g_Engine->Hwnd(), "Failed to create texture image view");
        return nullptr;
    }

    return imageView;
}