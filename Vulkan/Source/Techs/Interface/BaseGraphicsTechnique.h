#pragma once
#include "ITechnique.h"

class CBaseGraphicsTechnique : public ITechnique
{
public:
    CBaseGraphicsTechnique(ITechnique* parent = nullptr);

    // Init
    bool CreatePipeline() override;

protected:
    // Get Pipeline description
    void GetInputAssemblyDesc(VkPipelineInputAssemblyStateCreateInfo& inputAssembly) override;
    void GetViewportDesc(VkPipelineViewportStateCreateInfo& viewportState) override;
    void GetRasterizerDesc(VkPipelineRasterizationStateCreateInfo& rasterizer) override;
    void GetMultisamplingDesc(VkPipelineMultisampleStateCreateInfo& multisampling) override;
    void GetDepthStencilDesc(VkPipelineDepthStencilStateCreateInfo& depthStencil) override;
    void GetColorBlendDesc(VkPipelineColorBlendStateCreateInfo& colorBlending) override;
    void GetDynamicStateDesc(VkPipelineDynamicStateCreateInfo& dynamicState) override;
};