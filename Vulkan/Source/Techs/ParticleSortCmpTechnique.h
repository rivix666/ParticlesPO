#pragma once
#include "Interface/BaseComputeTechnique.h"

class CParticleSortCmpTechnique : public CBaseComputeTechnique
{
public:
    CParticleSortCmpTechnique(ITechnique* parent = nullptr);

protected:
    void GetShadersDesc(SShaderParams& params);
};

