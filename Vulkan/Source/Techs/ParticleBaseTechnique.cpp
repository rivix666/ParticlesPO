#include "stdafx.h"
#include "ParticleBaseTechnique.h"
#include "ShaderUtils.h"

VkVertexInputBindingDescription ParticleVertex::m_BindingDesc = {};
std::array<VkVertexInputAttributeDescription, 2> ParticleVertex::m_AttributeDesc = {};

VkVertexInputBindingDescription* ParticleVertex::GetBindingDescription()
{
    static bool prepared = false; // #TECH_UGH...
    if (!prepared)
    {
        m_BindingDesc.binding = 0;
        m_BindingDesc.stride = sizeof(ParticleVertex);
        m_BindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        prepared = true;
    }
    return &m_BindingDesc;
}

std::array<VkVertexInputAttributeDescription, 2>* ParticleVertex::GetAttributeDescriptions()
{
    static bool prepared = false; // #TECH_UGH... // moze szykowac je jak graphics pipeline czyli przychodza tutaj z gory
    if (!prepared)
    {
        // Pos
        m_AttributeDesc[0].binding = 0;
        m_AttributeDesc[0].location = 0; //#INPUT_VERTEX_WAZNElokacje musza sie zgadzac z shaderem
        m_AttributeDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT; //#INPUT_VERTEX_WAZNE pamietaj o tym 
        m_AttributeDesc[0].offset = offsetof(ParticleVertex, pos);

        // Life
        m_AttributeDesc[1].binding = 0;
        m_AttributeDesc[1].location = 1;
        m_AttributeDesc[1].format = VK_FORMAT_R32_SFLOAT;
        m_AttributeDesc[1].offset = offsetof(ParticleVertex, life);

        prepared = true;
    }
    return &m_AttributeDesc;
}

CParticleBaseTechnique::CParticleBaseTechnique()
    : ITechnique()
{

}

CParticleBaseTechnique::~CParticleBaseTechnique()
{
}

void CParticleBaseTechnique::GetInputAssemblyDesc(VkPipelineInputAssemblyStateCreateInfo& inputAssembly)
{
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void CParticleBaseTechnique::GetVertexInputDesc(VkPipelineVertexInputStateCreateInfo& vertexInputInfo)
{
    auto bindingDescription = ParticleVertex::GetBindingDescription();
    auto attributeDescriptions = ParticleVertex::GetAttributeDescriptions();
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions->size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions->data();
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