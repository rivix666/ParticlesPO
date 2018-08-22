#pragma once
#include "ITechnique.h"

struct ParticleVertex
{
    ParticleVertex() = default;

    glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);

    float life = 0.0f;
    // float burn, uint tech, float max_size, float frame

    static void GetBindingDescription(VkVertexInputBindingDescription& out_desc);
    static void GetAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& out_desc);
};

class CParticleBaseTechnique : public ITechnique
{
public:
    CParticleBaseTechnique();

    // Image getters
    void GetImageSamplerPairs(std::vector<TImgSampler>& out_pairs) const override;

    // Buffers handle
    bool CreateRenderObjects() override;
    void DestroyRenderObjects() override;

    // Images handle
    bool LoadImage();
    bool CreateImageView();
    bool CreateTextureSampler();

    // Static const variables
    static const VkFormat TEXTURE_FORMAT = VK_FORMAT_BC3_UNORM_BLOCK;

protected:
    void GetColorBlendDesc(VkPipelineColorBlendStateCreateInfo& colorBlending) override;
    void GetInputAssemblyDesc(VkPipelineInputAssemblyStateCreateInfo& inputAssembly) override;
    void GetVertexInputDesc(VkPipelineVertexInputStateCreateInfo& vertexInputInfo) override;
    void GetPipelineLayoutDesc(VkPipelineLayoutCreateInfo& pipelineLayoutInfo) override;
    void GetShadersDesc(SShaderParams& params) override;
    uint32_t GetRenderSubpassIndex() const override { return 1; } //#SUBPASSES

    // Images // #PARTICLES only temporary, particles will be stored in volumetric texture, so it will be created in particles manager or somewhere else
    uint32_t m_MipLevels = 0;
    VkImage m_TextureImage = nullptr;
    VkDeviceMemory m_TextureImageMemory = nullptr;

    // Image views
    VkSampler m_TextureSampler = nullptr;
    VkImageView m_TextureImageView = nullptr;

    // Vertices
    VkVertexInputBindingDescription m_BindingDesc;
    std::vector<VkVertexInputAttributeDescription> m_AttributesDesc;
};

