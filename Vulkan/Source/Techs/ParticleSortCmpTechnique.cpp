#include "stdafx.h"
#include "ParticleSortCmpTechnique.h"

CParticleSortCmpTechnique::CParticleSortCmpTechnique(ITechnique* parent /*= nullptr*/)
    : CBaseComputeTechnique(parent)
{
}

void CParticleSortCmpTechnique::GetShadersDesc(SShaderParams& params)
{
    params.compute_shader_path = "Effects/particleComp.spv";
    params.compute_entry = "main";
}