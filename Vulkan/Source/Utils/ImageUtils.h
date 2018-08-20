#pragma once

struct SImageParams
{
    std::string image_path = "";
    bool generate_mip_maps = false;
};

class CImageUtils
{
public:
    CImageUtils() = default;
    CImageUtils(const SImageParams& params);

    // Getters
    uint32_t MipLevels() const { return m_MipLevels; }
    VkImage TextureImage() const { return m_TextureImage; }
    VkDeviceMemory  TextureImageMemory() const { return m_TextureImageMemory; }

    // Load given image
    bool CreateTextureImage(const SImageParams& params);
    
    // Static Image methods
    static bool CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    static void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    static void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    // Static format methods
    static VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    // Static image view methods
    static VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

    // MipMaps
    static void GenerateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

private:
    uint32_t m_MipLevels = 0;
    VkImage m_TextureImage = nullptr;
    VkDeviceMemory m_TextureImageMemory = nullptr;
};

