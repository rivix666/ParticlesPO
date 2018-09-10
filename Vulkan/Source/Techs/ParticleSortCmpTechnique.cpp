#include "stdafx.h"
#include "ParticleSortCmpTechnique.h"

CParticleSortCmpTechnique::CParticleSortCmpTechnique(ITechnique* parent /*= nullptr*/)
    : CBaseComputeTechnique(parent)
{
}

bool CParticleSortCmpTechnique::CreateRenderObjects()
{
    // Create buffer views
    if (!g_Engine->Renderer()->CreateBufferView(g_Engine->ParticleMgr()->VertexBuffer(), VK_FORMAT_R32G32B32A32_SFLOAT, 0, VK_WHOLE_SIZE, m_BufferView))
        return false;

    // Register descriptor
    if (!g_Engine->Renderer()->DescMgr()->RegisterDescriptor(
        { m_BufferView },
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        VK_SHADER_STAGE_COMPUTE_BIT,
        (uint32_t)EDescSetRole::COMPUTE,
        g_Engine->Renderer()->DescMgr()->GetNextFreeLocationId((uint32_t)EDescSetRole::COMPUTE)))
        return false;

#ifdef _DEBUG
    // Debug vertex buffer
    g_Engine->Renderer()->CreateBuffer(
        PARTICLE_VERTEX_BUFF_SIZE,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_DebugBuffer,
        m_DebugBufferMemory);

    if (!g_Engine->Renderer()->CreateBufferView(m_DebugBuffer, VK_FORMAT_R32G32B32A32_SFLOAT, 0, VK_WHOLE_SIZE, m_DebugView))
        return false;

    // Register descriptor
    if (!g_Engine->Renderer()->DescMgr()->RegisterDescriptor(
        { m_DebugView },
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        VK_SHADER_STAGE_COMPUTE_BIT,
        (uint32_t)EDescSetRole::COMPUTE,
        g_Engine->Renderer()->DescMgr()->GetNextFreeLocationId((uint32_t)EDescSetRole::COMPUTE)))
        return false;
#endif

    return true;
}

void CParticleSortCmpTechnique::GetShadersDesc(SShaderParams& params)
{
    params.compute_shader_path = "Effects/particleComp.spv";
    params.compute_entry = "main";
}