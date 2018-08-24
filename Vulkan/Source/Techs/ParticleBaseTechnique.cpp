#include "stdafx.h"
#include "ParticleBaseTechnique.h"
#include "../Utils/ImageUtils.h"
#include "../DescriptorManager.h"

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

    // TechId
    VkVertexInputAttributeDescription tech_id_desc = {};
    life_desc.binding = 0;
    life_desc.location = 2;
    life_desc.format = VK_FORMAT_R32_SINT;
    life_desc.offset = offsetof(ParticleVertex, tech_id);
    out_desc.push_back(life_desc);
}

//////////////////////////////////////////////////////////////////////////
CParticleBaseTechnique::CParticleBaseTechnique(ITechnique* parent /*= nullptr*/)
    : CBaseGraphicsTechnique(parent)
{
}

bool CParticleBaseTechnique::CreateRenderObjects()
{
    return true;
}

void CParticleBaseTechnique::DestroyRenderObjects()
{
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
    vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)(m_AttributesDesc.size());
    vertexInputInfo.pVertexBindingDescriptions = &m_BindingDesc;
    vertexInputInfo.pVertexAttributeDescriptions = m_AttributesDesc.data();
}

void CParticleBaseTechnique::GetPipelineLayoutDesc(VkPipelineLayoutCreateInfo& pipelineLayoutInfo)
{
    static std::vector<VkDescriptorSetLayout> lays = { 
        g_Engine->DescMgr()->DescriptorSetLayout((uint32_t)EDescSetRole::GENERAL), 
        g_Engine->DescMgr()->DescriptorSetLayout((uint32_t)EDescSetRole::PARTICLES),
        g_Engine->DescMgr()->DescriptorSetLayout((uint32_t)EDescSetRole::DEPTH)
    };
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = (uint32_t)lays.size();
    pipelineLayoutInfo.pSetLayouts = lays.data();
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