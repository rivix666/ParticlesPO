#include "stdafx.h"
#include "BaseComputeTechnique.h"

CBaseComputeTechnique::CBaseComputeTechnique(ITechnique* parent /*= nullptr*/)
    : ITechnique(parent)
{
}

CBaseComputeTechnique::~CBaseComputeTechnique()
{
}

bool CBaseComputeTechnique::Init()
{
    return true;
}

bool CBaseComputeTechnique::Shutdown()
{
    return true;
}

bool CBaseComputeTechnique::CreatePipeline()
{
    if (m_Parent)
    {
        if (m_Parent->GetPipeline() && m_Parent->GetPipelineLayout())
            return true;
        return utils::FatalError(g_Engine->Hwnd(), "Invalid Technique parent");
    }

    // Shaders
    SShaderParams shaders_params;
    GetShadersDesc(shaders_params);
    CShaderUtils shadersMgr(shaders_params);

    // Pipeline layout
    if (!CreatePipelineLayout())
        return false;

    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = {};
    pipelineInfo.stage = shadersMgr.GetShaderStageInfoVec().front();
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (VKRESULT(vkCreateComputePipelines(m_Renderer->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create compute pipeline");

    return true;
}

void CBaseComputeTechnique::GetPipelineLayoutDesc(VkPipelineLayoutCreateInfo& pipelineLayoutInfo)
{
    static std::vector<VkDescriptorSetLayout> lays = { g_Engine->DescMgr()->DescriptorSetLayout((uint32_t)EDescSetRole::GENERAL)};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = (uint32_t)lays.size();
    pipelineLayoutInfo.pSetLayouts = lays.data();
}