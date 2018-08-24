#include "stdafx.h"
#include "ParticleSortCmpTechnique.h"

CParticleSortCmpTechnique::CParticleSortCmpTechnique(ITechnique* parent /*= nullptr*/)
    : CBaseComputeTechnique(parent)
{
}

bool CParticleSortCmpTechnique::CreateRenderObjects()
{
    if (!g_Engine->Renderer()->CreateBufferView(g_Engine->ParticleMgr()->VertexBuffer(), VK_FORMAT_R32G32B32A32_SFLOAT, 0, VK_WHOLE_SIZE, m_BufferView))
        return false;

    if (!g_Engine->Renderer()->DescMgr()->RegisterDescriptor(
        { m_BufferView },
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        VK_SHADER_STAGE_COMPUTE_BIT,
        (uint32_t)EDescSetRole::COMPUTE,
        g_Engine->Renderer()->DescMgr()->GetNextFreeLocationId((uint32_t)EDescSetRole::COMPUTE)))
        return false;

    return true;
}

void CParticleSortCmpTechnique::GetShadersDesc(SShaderParams& params)
{
    params.compute_shader_path = "Effects/particleComp.spv";
    params.compute_entry = "main";
}