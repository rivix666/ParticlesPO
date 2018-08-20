#pragma once
#include "ITechnique.h"

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

class CBaseTechnique : public ITechnique
{
public:
    CBaseTechnique() = default;

    // UniBuff getters
    VkBuffer UniBuffer() const { return m_BaseObjUniBuffer; }
    VkDeviceMemory UniBufferMemory() const { return m_BaseObjUniBufferMemory; }
    size_t GetSingleUniBuffObjSize() const { return sizeof(SObjUniBuffer); }

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

protected:
    void GetVertexInputDesc(VkPipelineVertexInputStateCreateInfo& vertexInputInfo) override;
    void GetShadersDesc(SShaderParams& params) override;

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

