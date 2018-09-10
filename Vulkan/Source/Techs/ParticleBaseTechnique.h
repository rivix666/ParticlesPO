#pragma once
#include "Interface/BaseGraphicsTechnique.h"

struct SParticleTechUniBuffer
{
    float burn = 1.0f;
    float max_size = 1.0f;
    int   texture_id = 6; // EParticleTex::COLORS;
};

struct ParticleVertex
{
    ParticleVertex() = default;

    glm::vec3   pos = glm::vec3(0.0f, 0.0f, 0.0f);
    float       life = 0.0f;
    float       tech_id = 0;

    glm::vec3   dummy_vec = glm::vec3(-1.0f, 0.0f, 666.0f);

    static void GetBindingDescription(VkVertexInputBindingDescription& out_desc);
    static void GetAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& out_desc);
};

// #PARTICLES #TECH Dorobiæ mo¿liwoœæ wspó³dzielenia pipelinu przez kilka technik, wtedy bym unkn¹³ problemów z sortowaniem
class CParticleBaseTechnique : public CBaseGraphicsTechnique
{
public:
    CParticleBaseTechnique(ITechnique* parent = nullptr);

    // Buffers handle
    bool CreateRenderObjects() override;
    void DestroyRenderObjects() override;

    const SParticleTechUniBuffer& GetParticleUniBuffData() const { return m_UniBuffData; }

protected:
    // Init
    void GetColorBlendDesc(VkPipelineColorBlendStateCreateInfo& colorBlending) override;
    void GetInputAssemblyDesc(VkPipelineInputAssemblyStateCreateInfo& inputAssembly) override;
    void GetVertexInputDesc(VkPipelineVertexInputStateCreateInfo& vertexInputInfo) override;
    void GetPipelineLayoutDesc(VkPipelineLayoutCreateInfo& pipelineLayoutInfo) override;
    void GetShadersDesc(SShaderParams& params) override;
    uint32_t GetRenderSubpassIndex() const override { return 1; } //#SUBPASSES

    // Vertices
    VkVertexInputBindingDescription m_BindingDesc;
    std::vector<VkVertexInputAttributeDescription> m_AttributesDesc;

    SParticleTechUniBuffer m_UniBuffData;
};