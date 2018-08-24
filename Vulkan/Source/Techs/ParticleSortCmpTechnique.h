#pragma once
#include "Interface/BaseComputeTechnique.h"

class CParticleSortCmpTechnique : public CBaseComputeTechnique
{
public:
    CParticleSortCmpTechnique(ITechnique* parent = nullptr);

    // Buffers handle
    bool CreateRenderObjects() override;

protected:
    void GetShadersDesc(SShaderParams& params);

    // Views
    VkBufferView m_BufferView = nullptr;
};

