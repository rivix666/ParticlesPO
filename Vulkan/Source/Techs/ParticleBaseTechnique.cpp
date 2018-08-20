#include "stdafx.h"
#include "ParticleBaseTechnique.h"
#include "../Utils/ImageUtils.h"

void ParticleVertex::GetBindingDescription(VkVertexInputBindingDescription& out_desc)
{
    out_desc.binding = 0;
    out_desc.stride = sizeof(ParticleVertex);
    out_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void ParticleVertex::GetAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& out_desc)
{
    // Pos
    VkVertexInputAttributeDescription pos_desc = {};
    pos_desc.binding = 0;
    pos_desc.location = 0;
    pos_desc.format = VK_FORMAT_R32G32B32_SFLOAT;
    pos_desc.offset = offsetof(ParticleVertex, pos);
    out_desc.push_back(pos_desc);

    // Life
    VkVertexInputAttributeDescription life_desc = {};
    life_desc.binding = 0;
    life_desc.location = 1;
    life_desc.format = VK_FORMAT_R32_SFLOAT;
    life_desc.offset = offsetof(ParticleVertex, life);
    out_desc.push_back(life_desc);
}

//////////////////////////////////////////////////////////////////////////
CParticleBaseTechnique::CParticleBaseTechnique()
    : ITechnique()
{
}

void CParticleBaseTechnique::GetImageSamplerPairs(std::vector<TImgSampler>& out_pairs) const
{
    out_pairs.push_back(TImgSampler(m_TextureImageView, m_TextureSampler));
}

bool CParticleBaseTechnique::CreateRenderObjects()
{
    if (!LoadImage())
        return false;

    if (!CreateImageView())
        return false;

    if (!CreateTextureSampler())
        return false;

    return true;
}

void CParticleBaseTechnique::DestroyRenderObjects()
{
    // Destroy Images
    if (m_TextureImage)
    {
        vkDestroyImage(g_Engine->Renderer()->GetDevice(), m_TextureImage, nullptr);
        m_TextureImage = nullptr;
    }
    if (m_TextureImageMemory)
    {
        vkFreeMemory(g_Engine->Renderer()->GetDevice(), m_TextureImageMemory, nullptr);
        m_TextureImageMemory = nullptr;
    }

    // Destroy texture sampler and image view
    if (m_TextureSampler)
    {
        vkDestroySampler(g_Engine->Renderer()->GetDevice(), m_TextureSampler, nullptr);
        m_TextureSampler = nullptr;
    }
    if (m_TextureImageView)
    {
        vkDestroyImageView(g_Engine->Renderer()->GetDevice(), m_TextureImageView, nullptr);
        m_TextureImageView = nullptr;
    }
}

bool CParticleBaseTechnique::LoadImage()
{
    SImageParams params = { "Images/Particles/Tex/tmp_tex.png", true };
    CImageUtils img_utils(params);

    m_TextureImage = img_utils.TextureImage();
    m_TextureImageMemory = img_utils.TextureImageMemory();
    m_MipLevels = img_utils.MipLevels();

    if (!m_TextureImage || !m_TextureImageMemory)
        return utils::FatalError(g_Engine->Hwnd(), "Failed to initialaze CBaseTechnique");

    return true;
}

bool CParticleBaseTechnique::CreateImageView()
{
    m_TextureImageView = CImageUtils::CreateImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);
    return m_TextureImageView != nullptr;
}

bool CParticleBaseTechnique::CreateTextureSampler()
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0; // Optional
    samplerInfo.maxLod = static_cast<float>(m_MipLevels);
    samplerInfo.mipLodBias = 0; // Optional

    if (VKRESULT(vkCreateSampler(g_Engine->Renderer()->GetDevice(), &samplerInfo, nullptr, &m_TextureSampler)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create texture sampler");

    return true;
}

void CParticleBaseTechnique::GetColorBlendDesc(VkPipelineColorBlendStateCreateInfo& colorBlending)
{
    __super::GetColorBlendDesc(colorBlending);

    static VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    colorBlending.pAttachments = &colorBlendAttachment;
}

void CParticleBaseTechnique::GetInputAssemblyDesc(VkPipelineInputAssemblyStateCreateInfo& inputAssembly)
{
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void CParticleBaseTechnique::GetVertexInputDesc(VkPipelineVertexInputStateCreateInfo& vertexInputInfo)
{
    // Prepare descriptions
    m_AttributesDesc.clear();
    ParticleVertex::GetBindingDescription(m_BindingDesc);
    ParticleVertex::GetAttributeDescriptions(m_AttributesDesc);

    // Fill vertex input info
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_AttributesDesc.size());
    vertexInputInfo.pVertexBindingDescriptions = &m_BindingDesc;
    vertexInputInfo.pVertexAttributeDescriptions = m_AttributesDesc.data();
}

void CParticleBaseTechnique::GetShadersDesc(SShaderParams& params)
{
    params.vertex_shader_path = "Effects/particleVert.spv";
    params.geometry_shader_path = "Effects/particleGeom.spv";
    params.fragment_shader_path = "Effects/particleFrag.spv";
    params.vertex_entry = "main";
    params.geometry_entry = "main";
    params.fragment_entry = "main";
}