#pragma once
#include "ITechnique.h"

// #PARTICLES bez kombinowania wszystkie particle bêd¹ u¿ywaæ tego opisu
struct ParticleVertex
{
    ParticleVertex() = default;

    glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);

    float life = 0.0f;
    // float burn, uint tech, float max_size, float frame

    static VkVertexInputBindingDescription m_BindingDesc;
    static std::array<VkVertexInputAttributeDescription, 2> m_AttributeDesc;

    static VkVertexInputBindingDescription* GetBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2>* GetAttributeDescriptions();
};

class CParticleBaseTechnique : public ITechnique
{
public:
    CParticleBaseTechnique();
    ~CParticleBaseTechnique();

protected:
    void GetInputAssemblyDesc(VkPipelineInputAssemblyStateCreateInfo& inputAssembly) override;
    void GetVertexInputDesc(VkPipelineVertexInputStateCreateInfo& vertexInputInfo) override;
    void GetShadersDesc(SShaderParams& params) override;
};

