#include "stdafx.h"
#include "ITechnique.h"

ITechnique::ITechnique(ITechnique* parent /*= nullptr*/)
    : m_Parent(parent)
{
}

ITechnique::~ITechnique()
{
    DestroyRenderObjects();
}

bool ITechnique::Init()
{
    m_Renderer = g_Engine->Renderer();

    if (!CreatePipeline())
        return Shutdown();

    return true;
}

bool ITechnique::Shutdown()
{
    if (m_Renderer)
    {
        if (m_Pipeline)
        {
            vkDestroyPipeline(m_Renderer->GetDevice(), m_Pipeline, nullptr);
            m_Pipeline = nullptr;
        }

        if (m_PipelineLayout)
        {
            vkDestroyPipelineLayout(m_Renderer->GetDevice(), m_PipelineLayout, nullptr);
            m_PipelineLayout = nullptr;
        }

        m_Renderer = nullptr;
    }

    return false;
}

VkPipeline ITechnique::GetPipeline() const
{
    return m_Parent ? m_Parent->GetPipeline() : m_Pipeline;
}

VkPipelineLayout ITechnique::GetPipelineLayout() const
{
    return m_Parent ? m_Parent->GetPipelineLayout() : m_PipelineLayout;
}

bool ITechnique::CreatePipelineLayout()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    GetPipelineLayoutDesc(pipelineLayoutInfo);

    if (VKRESULT(vkCreatePipelineLayout(g_Engine->Renderer()->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create pipeline layout");

    return true;
}