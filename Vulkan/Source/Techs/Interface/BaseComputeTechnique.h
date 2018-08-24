#pragma once
#include "ITechnique.h"

class CBaseComputeTechnique : public ITechnique
{
public:
    CBaseComputeTechnique(ITechnique* parent = nullptr);
    virtual ~CBaseComputeTechnique();

    // Init
    bool                CreatePipeline() override;

protected:
    // Subpasses
    uint32_t            GetRenderSubpassIndex() const override { return 0; }

    // Pipeline Description
    void                GetPipelineLayoutDesc(VkPipelineLayoutCreateInfo& pipelineLayoutInfo) override;
};

