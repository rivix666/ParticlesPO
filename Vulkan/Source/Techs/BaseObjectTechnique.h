#pragma once
#include "Interface/BaseGraphicsTechnique.h"

struct SObjUniBuffer
{
    glm::mat4 obj_world;
    float tex_mul;
};

struct BaseVertex
{
    BaseVertex() = default;

    glm::vec3 pos;
    glm::vec2 texCoord;

    static void GetBindingDescription(VkVertexInputBindingDescription& out_desc);
    static void GetAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& out_desc);
};

class CBaseObjectTechnique : public CBaseGraphicsTechnique
{
public:
    CBaseObjectTechnique() = default;

    // UniBuff getters
    VkBuffer UniBuffer() const override { return m_BaseObjUniBuffer; }
    VkDeviceMemory UniBufferMemory() const override { return m_BaseObjUniBufferMemory; }
    size_t GetSingleUniBuffObjSize() const override { return sizeof(SObjUniBuffer); }

    // Image getters
    void GetImageSamplerPairs(std::vector<TImgSampler>& out_pairs) const override;

    // Buffers handle
    bool CreateRenderObjects() override;
    void DestroyRenderObjects() override;

    bool CreateUniBuffer();

    // Images handle
    bool LoadImage();
    bool CreateImageView();
    bool CreateTextureSampler();

    // Static const variables
    static const VkFormat TEXTURE_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;

protected:
    void GetVertexInputDesc(VkPipelineVertexInputStateCreateInfo& vertexInputInfo) override;
    void GetPipelineLayoutDesc(VkPipelineLayoutCreateInfo& pipelineLayoutInfo) override;
    void GetShadersDesc(SShaderParams& params) override;
    uint32_t GetRenderSubpassIndex() const override { return 0; } //#SUBPASSES

private:
    // Uni buffers
    VkBuffer m_BaseObjUniBuffer = nullptr;
    VkDeviceMemory m_BaseObjUniBufferMemory = nullptr;

    // Vertices
    VkVertexInputBindingDescription m_BindingDesc;
    std::vector<VkVertexInputAttributeDescription> m_AttributesDesc;

    // Images
    uint32_t m_MipLevels = 0;
    VkImage m_TextureImage = nullptr;
    VkDeviceMemory m_TextureImageMemory = nullptr;

    // Image views
    VkSampler m_TextureSampler = nullptr;
    VkImageView m_TextureImageView = nullptr;
};

