#pragma once

namespace image_utils
{
    struct SImageParams
    {
        // Input
        std::string         in_image_path = "";
        bool                in_generate_mip_maps = false;

        // Output
        uint32_t            out_mip_levels = 0;
        VkImage             out_texture_image = nullptr;
        VkFormat            out_texture_format = VK_FORMAT_UNDEFINED;
        VkDeviceMemory      out_texture_image_memory = nullptr;
    };

    // Load given image
    bool CreateTextureImage(SImageParams& params);
    
    // Image methods
    bool CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t mip_level = 0);

    // Format methods
    VkFormat MapGliFormat2VkFormat(int gli_format_type);
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    // Image view methods
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
};

