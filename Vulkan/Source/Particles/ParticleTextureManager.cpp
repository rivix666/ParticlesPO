#include "stdafx.h"
#include "ParticleTextureManager.h"
#include "../DescriptorManager.h"
#include "../Utils/ImageUtils.h"

SParticleTex::~SParticleTex()
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
    if (m_Sampler)
    {
        vkDestroySampler(g_Engine->Renderer()->GetDevice(), m_Sampler, nullptr);
        m_Sampler = nullptr;
    }
    if (m_ImageView)
    {
        vkDestroyImageView(g_Engine->Renderer()->GetDevice(), m_ImageView, nullptr);
        m_ImageView = nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////
CParticleTextureManager::CParticleTextureManager()
{
    PrepareVectors();
}

bool CParticleTextureManager::Init()
{
    if (!LoadParticleTextures())
        return Shutdown();

    if (!RegisterImageViews())
        return Shutdown();

    if (!CreateUniBuffers())
        return Shutdown();

    if (!RegisterUniBuffs())
        return Shutdown();

    return true;
}

bool CParticleTextureManager::Shutdown()
{
    m_Textures.clear();
    ReleaseUniBuffers();
    return false;
}

void CParticleTextureManager::RegisterBaseTextures()
{
    REGISTER_P_TEX(Flame, 8.0f, 7.0f);
    REGISTER_P_TEX(Smoke, 8.0f, 8.0f);
    REGISTER_P_TEX(Debris, 3.0f, 3.0f);
    REGISTER_P_TEX(Flare, 1.0f, 1.0f);
    REGISTER_P_TEX(HaloFlare, 8.0f, 8.0f);
    REGISTER_P_TEX(RoundSparks, 1.0f, 1.0f);
    REGISTER_P_TEX(COLORS, 1.0f, 8.0f);
}

void CParticleTextureManager::RegisterTexture(const std::string& path, const EParticleTex& type, const float& w_count, const float& h_count)
{
    m_Textures[(uint32_t)type].m_TexturePath = path;
    m_Textures[(uint32_t)type].m_AtlasWidth = w_count;
    m_Textures[(uint32_t)type].m_AtlasHeight = h_count;
}

const SParticleTex& CParticleTextureManager::GetTexture(const EParticleTex& type) const
{
    return m_Textures[(uint32_t)type];
}

void CParticleTextureManager::PrepareVectors()
{
    if (!m_Textures.empty())
    {
        m_Textures.clear();
        ReleaseUniBuffers();
    }

    m_Textures.resize((size_t)EParticleTex::_COUNT_);
    m_UniBuffs.resize((size_t)EParticleTex::_COUNT_);
    m_UniBuffsMem.resize((size_t)EParticleTex::_COUNT_);
}

bool CParticleTextureManager::LoadParticleTextures()
{
    for (uint32_t i = 0; i < (uint32_t)EParticleTex::_COUNT_; i++)
    {
        if (!LoadParticleTexture((EParticleTex)i))
            return false;
        if (!CreateTextureView((EParticleTex)i))
            return false;
        if (!CreateTextureSampler((EParticleTex)i))
            return false;
    }
    return true;
}

bool CParticleTextureManager::LoadParticleTexture(const EParticleTex& type)
{
    auto& tex = m_Textures[(uint32_t)type];
    image_utils::SImageParams params = { tex.m_TexturePath, false };
    image_utils::CreateTextureImage(params);

    tex.m_TextureImage = params.out_texture_image;
    tex.m_TextureImageMemory = params.out_texture_image_memory;
    tex.m_MipLevels = params.out_mip_levels;
    tex.m_TextureFormat = params.out_texture_format;

    if (!tex.m_TextureImage || !tex.m_TextureImageMemory)
        return utils::FatalError(g_Engine->Hwnd(), "Failed to load particle textures");
}

bool CParticleTextureManager::CreateTextureView(const EParticleTex& type)
{
    auto& tex = m_Textures[(uint32_t)type];
    tex.m_ImageView = image_utils::CreateImageView(tex.m_TextureImage, tex.m_TextureFormat, VK_IMAGE_ASPECT_COLOR_BIT, tex.m_MipLevels);
    return tex.m_ImageView != nullptr;
}

bool CParticleTextureManager::CreateTextureSampler(const EParticleTex& type)
{
    auto& tex = m_Textures[(uint32_t)type];

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
    samplerInfo.maxLod = static_cast<float>(tex.m_MipLevels);
    samplerInfo.mipLodBias = 0; // Optional

    if (VKRESULT(vkCreateSampler(g_Engine->Renderer()->GetDevice(), &samplerInfo, nullptr, &tex.m_Sampler)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create texture sampler");

    return true;
}

bool CParticleTextureManager::RegisterImageViews()
{
    // Prepare images data
    std::vector<ITechnique::TImgSampler> img_pairs;
    for (uint32_t i = 0; i < (uint32_t)EParticleTex::_COUNT_; i++)
    {
        auto& tex = m_Textures[i];
        img_pairs.push_back(ITechnique::TImgSampler(tex.m_ImageView, tex.m_Sampler));
    }

    // Register images array in Descriptor Set
    if (!g_Engine->Renderer()->DescMgr()->RegisterDescriptor(
        img_pairs, {},
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        (uint32_t)EDescSetRole::PARTICLES,
        g_Engine->Renderer()->DescMgr()->GetNextFreeLocationId((uint32_t)EDescSetRole::PARTICLES)))
        return false;

    // Register EParticleTex::COLORS as image view in Geometry Shader (so we can took alpha and use it as particle size)
    if (!g_Engine->Renderer()->DescMgr()->RegisterDescriptor(
        { ITechnique::TImgSampler(m_Textures[EParticleTex::COLORS].m_ImageView, m_Textures[EParticleTex::COLORS].m_Sampler) }, {},
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        VK_SHADER_STAGE_GEOMETRY_BIT,
        (uint32_t)EDescSetRole::PARTICLES,
        g_Engine->Renderer()->DescMgr()->GetNextFreeLocationId((uint32_t)EDescSetRole::PARTICLES)))
        return false;

    return true;
}

bool CParticleTextureManager::RegisterUniBuffs()
{
    // Store sizes of individual elements
    size_t offset = 0;
    std::vector<size_t> sizes;
    for (size_t i = 0; i < (uint32_t)EParticleTex::_COUNT_; i++)
    {
        sizes.push_back(g_Engine->Renderer()->GetUniBuffObjSize(sizeof(SParticleTexUniBuffer)));
    }

    // Register images array in Descriptor Set
    if (!g_Engine->Renderer()->DescMgr()->RegisterDescriptor(
        m_UniBuffs, sizes,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_SHADER_STAGE_GEOMETRY_BIT,
        (uint32_t)EDescSetRole::PARTICLES,
        g_Engine->Renderer()->DescMgr()->GetNextFreeLocationId((uint32_t)EDescSetRole::PARTICLES)))
        return false;

    return true;
}

bool CParticleTextureManager::CreateUniBuffers()
{
    for (uint32_t i = 0; i < (uint32_t)EParticleTex::_COUNT_; i++)
    {
        if (!CreateTexUniBuffer((EParticleTex)i))
            return false;
    }
    return true;
}

bool CParticleTextureManager::CreateTexUniBuffer(const EParticleTex& type)
{
    // Create Buffer
    size_t minUboAlignment = g_Engine->Renderer()->MinUboAlignment();
    double size = ceil((double)sizeof(SParticleTexUniBuffer) / (double)minUboAlignment);
    VkDeviceSize bufferSize = minUboAlignment * size * EParticleTex::_COUNT_;

    if (!g_Engine->Renderer()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniBuffs[(uint32_t)type], m_UniBuffsMem[(uint32_t)type]))
        return false;

    // Copy Data into it
    auto& tex = m_Textures[(uint32_t)type];
    SParticleTexUniBuffer ub = { tex.m_AtlasWidth, tex.m_AtlasHeight };

    void* data;
    vkMapMemory(g_Engine->Device(), m_UniBuffsMem[(uint32_t)type], 0, sizeof(ub), 0, &data);
    memcpy(data, &ub, sizeof(ub));
    vkUnmapMemory(g_Engine->Device(), m_UniBuffsMem[(uint32_t)type]);
    return true;
}

void CParticleTextureManager::ReleaseUniBuffers()
{
    for (auto& ubo : m_UniBuffs)
    {
        if (ubo)
        {
            vkDestroyBuffer(g_Engine->Renderer()->GetDevice(), ubo, nullptr);
        }
    }

    for (auto& mem : m_UniBuffsMem)
    {
        if (mem)
        {
            vkFreeMemory(g_Engine->Renderer()->GetDevice(), mem, nullptr);
        }
    }
}

// COPY ONE BIG BUFFER #DESC_MGR
//////////////////////////////////////////////////////////////////////////
// COPY ONE BIG BUFFER
// size_t offset = 0;
// for (size_t arr = 0; arr < data.sizes.size(); arr++)
// {
//     VkDescriptorBufferInfo info;
//     info.buffer = data.buffer[0];
//     info.offset = offset;
//     info.range = data.sizes[arr];
//     out_vec.push_back(info);
//     offset += data.sizes[arr];
// }
// bool CParticleTextureManager::CreateUniBuffers()
// {
//     //for (uint32_t i = 0; i < (uint32_t)EParticleTex::_COUNT_; i++)
//     //{
//     if (!CreateTexUniBuffer((EParticleTex)0/*i*/))
//         return false;
//     //}
//     return true;
// }
// 
// bool CParticleTextureManager::CreateTexUniBuffer(const EParticleTex& type)
// {
//     // VkDeviceSize bufferSize = g_Engine->Renderer()->GetUniBuffObjSize(sizeof(SParticleTexUniBuffer));
// 
// 
// 
//      // Create Buffer
//     size_t minUboAlignment = g_Engine->Renderer()->MinUboAlignment();
//     double size = ceil((double)sizeof(SParticleTexUniBuffer) / (double)minUboAlignment);
//     VkDeviceSize bufferSize = minUboAlignment * size * EParticleTex::_COUNT_;
// 
//     if (!g_Engine->Renderer()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniBuffs[(uint32_t)type], m_UniBuffsMem[(uint32_t)type]))
//         return false;
// 
//     // Copy Data into it
//     std::vector<SParticleTexUniBuffer> kokoszka;
//     for (uint32_t i = 0; i < (uint32_t)EParticleTex::_COUNT_; i++)
//     {
//         auto& tex = m_Textures[i];
//         SParticleTexUniBuffer ub = { tex.m_AtlasWidth, tex.m_AtlasHeight };
//         kokoszka.push_back(ub);
//     }
// 
// 
// 
//     void* data;
//     size_t offset = 0;
//     vkMapMemory(g_Engine->Device(), m_UniBuffsMem[(uint32_t)type], 0, bufferSize, 0, &data);
//     for (uint32_t i = 0; i < (uint32_t)EParticleTex::_COUNT_; i++)
//     {
//         memcpy((SParticleTexUniBuffer*)data + offset, &kokoszka[i], g_Engine->Renderer()->GetUniBuffObjSize(sizeof(SParticleTexUniBuffer)));
//         offset += g_Engine->Renderer()->GetUniBuffObjSize(sizeof(SParticleTexUniBuffer));
//     }
//     vkUnmapMemory(g_Engine->Device(), m_UniBuffsMem[(uint32_t)type]);
//     return true;
// }
//////////////////////////////////////////////////////////////////////////