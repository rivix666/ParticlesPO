#include "stdafx.h"
#include "BaseObjectTechnique.h"
#include "../Utils/ImageUtils.h"
#include "../DescriptorManager.h"

void BaseVertex::GetBindingDescription(VkVertexInputBindingDescription& out_desc)
{
    out_desc.binding = 0;
    out_desc.stride = sizeof(BaseVertex);
    out_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void BaseVertex::GetAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& out_desc)
{
    // Pos
    VkVertexInputAttributeDescription pos_desc = {};
    pos_desc.binding = 0;
    pos_desc.location = 0;
    pos_desc.format = VK_FORMAT_R32G32B32_SFLOAT;
    pos_desc.offset = offsetof(BaseVertex, pos);
    out_desc.push_back(pos_desc);

    // TexCoord
    VkVertexInputAttributeDescription tex_coord_desc = {};
    tex_coord_desc.binding = 0;
    tex_coord_desc.location = 1;
    tex_coord_desc.format = VK_FORMAT_R32G32_SFLOAT;
    tex_coord_desc.offset = offsetof(BaseVertex, texCoord);
    out_desc.push_back(tex_coord_desc);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObjectTechnique::GetImageSamplerPairs(std::vector<TImgSampler>& out_pairs) const
{
    out_pairs.push_back(TImgSampler(m_TextureImageView, m_TextureSampler));
}

bool CBaseObjectTechnique::CreateRenderObjects()
{
    if (!LoadImage())
        return false;

    if (!CreateImageView())
        return false;

    if (!CreateTextureSampler())
        return false;

    if (!CreateUniBuffer())
        return false;

    // Register buffers in Descriptor Set
    std::vector<VkBuffer> buffers = { m_BaseObjUniBuffer };
    std::vector<size_t> sizes = { sizeof(SObjUniBuffer) };
    if (!g_Engine->Renderer()->DescMgr()->RegisterDescriptor(
        buffers, sizes,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        VK_SHADER_STAGE_VERTEX_BIT,
        (uint32_t)EDescSetRole::OBJECTS,
        g_Engine->Renderer()->DescMgr()->GetNextFreeLocationId((uint32_t)EDescSetRole::OBJECTS)))
        return false;

    // Register images in Descriptor Set
    std::vector<TImgSampler> img_pairs = { TImgSampler(m_TextureImageView, m_TextureSampler) };
    if (!g_Engine->Renderer()->DescMgr()->RegisterDescriptor(
        img_pairs, {},
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        (uint32_t)EDescSetRole::OBJECTS,
        g_Engine->Renderer()->DescMgr()->GetNextFreeLocationId((uint32_t)EDescSetRole::OBJECTS)))
        return false;

    return true;
}

void CBaseObjectTechnique::DestroyRenderObjects()
{
    // Destroy uni buffer
    if (m_BaseObjUniBuffer)
    {
        vkDestroyBuffer(g_Engine->Renderer()->GetDevice(), m_BaseObjUniBuffer, nullptr);
        m_BaseObjUniBuffer = nullptr;
    }
    if (m_BaseObjUniBufferMemory)
    {
        vkFreeMemory(g_Engine->Renderer()->GetDevice(), m_BaseObjUniBufferMemory, nullptr);
        m_BaseObjUniBufferMemory = nullptr;
    }

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

bool CBaseObjectTechnique::CreateUniBuffer()
{
    size_t minUboAlignment = g_Engine->Renderer()->MinUboAlignment();
    double size = ceil((double)GetSingleUniBuffObjSize() / (double)minUboAlignment);
    VkDeviceSize baseObjBufferSize = minUboAlignment * size * OBJ_PER_TECHNIQUE;
    return g_Engine->Renderer()->CreateBuffer(baseObjBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_BaseObjUniBuffer, m_BaseObjUniBufferMemory);
}

bool CBaseObjectTechnique::LoadImage()
{
    image_utils::SImageParams params = { "Images/Other/ground.dds", true };
    image_utils::CreateTextureImage(params);

    m_TextureImage = params.out_texture_image;
    m_TextureImageMemory = params.out_texture_image_memory;
    m_MipLevels = params.out_mip_levels;

    if (!m_TextureImage || !m_TextureImageMemory)
        return utils::FatalError(g_Engine->Hwnd(), "Failed to initialaze CBaseTechnique");

    return true;
}

bool CBaseObjectTechnique::CreateImageView()
{
    m_TextureImageView = image_utils::CreateImageView(m_TextureImage, TEXTURE_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);
    return m_TextureImageView != nullptr;
}

bool CBaseObjectTechnique::CreateTextureSampler()
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

void CBaseObjectTechnique::GetVertexInputDesc(VkPipelineVertexInputStateCreateInfo& vertexInputInfo)
{
    // Prepare descriptions
    m_AttributesDesc.clear();
    BaseVertex::GetBindingDescription(m_BindingDesc);
    BaseVertex::GetAttributeDescriptions(m_AttributesDesc);

    // Fill vertex input info
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)(m_AttributesDesc.size());
    vertexInputInfo.pVertexBindingDescriptions = &m_BindingDesc;
    vertexInputInfo.pVertexAttributeDescriptions = m_AttributesDesc.data();
}

void CBaseObjectTechnique::GetPipelineLayoutDesc(VkPipelineLayoutCreateInfo& pipelineLayoutInfo)
{
    static std::vector<VkDescriptorSetLayout> lays = { g_Engine->DescMgr()->DescriptorSetLayout((uint32_t)EDescSetRole::GENERAL), g_Engine->DescMgr()->DescriptorSetLayout((uint32_t)EDescSetRole::OBJECTS) };
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = (uint32_t)lays.size();
    pipelineLayoutInfo.pSetLayouts = lays.data();
}

void CBaseObjectTechnique::GetShadersDesc(SShaderParams& params)
{
    params.vertex_shader_path = "Effects/baseVert.spv";
    params.fragment_shader_path = "Effects/baseFrag.spv";
    params.vertex_entry = "main";
    params.fragment_entry = "main";
}
