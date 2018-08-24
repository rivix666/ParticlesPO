#pragma once

#define REGISTER_P_TEX(type, width, height) g_Engine->ParticleTexMgr()->RegisterTexture("Images/Particles/"#type".dds", type, width, height);

enum EParticleTex
{
    Flame = 0,
    Smoke,
    Debris,
    Flare,
    HaloFlare,
    RoundSparks,

    COLORS, // Needs to be last (it's hardcoded in shaders)

    _COUNT_
};

struct SParticleTex
{
    SParticleTex() = default;
    ~SParticleTex();

    // Texture
    VkImage         m_TextureImage = nullptr;
    VkDeviceMemory  m_TextureImageMemory = nullptr;

    // Texture Desc
    uint32_t        m_MipLevels = 0;
    VkFormat        m_TextureFormat = VK_FORMAT_UNDEFINED;
    std::string     m_TexturePath = "";

    // Atlas Desc
    float           m_AtlasWidth = 0;
    float           m_AtlasHeight = 0;

    // Image View
    VkImageView     m_ImageView = nullptr;
    VkSampler       m_Sampler = nullptr;
};

struct SParticleTexUniBuffer
{
    // Atlas Desc
    float           atlas_width = 0;
    float           atlas_height = 0;
};

class CParticleTextureManager
{
public:
    CParticleTextureManager();

    // Init/Release
    bool Init();
    bool Shutdown();

    // Textures Register
    void RegisterBaseTextures();
    void RegisterTexture(const std::string& path, const EParticleTex& type, const float& w_count, const float& h_count);

    // Getters
    const SParticleTex& GetTexture(const EParticleTex& type) const;

    // Typedefs
    typedef std::vector<SParticleTex>   TParticleTex;
    typedef std::vector<VkBuffer>       TBufferVec;
    typedef std::vector<VkDeviceMemory> TBufferMemVec;

protected:
    // Textures
    void PrepareVectors();
    bool LoadParticleTextures();
    bool LoadParticleTexture(const EParticleTex& type);
    bool CreateTextureView(const EParticleTex& type);
    bool CreateTextureSampler(const EParticleTex& type);

    // Descriptor Sets
    bool RegisterImageViews();
    bool RegisterUniBuffs();

    // Uni Buffers
    bool CreateUniBuffers();
    bool CreateTexUniBuffer(const EParticleTex& type);
    void ReleaseUniBuffers();

private:
    // Textures
    TParticleTex    m_Textures;

    // Uni Buffers
    TBufferVec      m_UniBuffs; // #DESC_MGR REWRITE
    TBufferMemVec   m_UniBuffsMem; // #DESC_MGR REWRITE
};

