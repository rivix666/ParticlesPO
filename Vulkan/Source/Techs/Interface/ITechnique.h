#pragma once
#include "../Utils/ShaderUtils.h"

class CVulkanRenderer;

class ITechnique // #TECH to powinno byæ stricte interfejsem i po tym dziedzicyæ dopeiro powinno BaseGraphicsTechnique i BaseComputeTechnique
{
public:
    // If Technique has a parent, It will use parent pipeline to record command buffer // #TECH_PARENT secure it later (crashes on shutdown, unregister etc.)
    ITechnique(ITechnique* parent = nullptr);
    virtual ~ITechnique();

    // Typedefs
    typedef std::pair<VkImageView, VkSampler> TImgSampler;

    // Init
    virtual bool                Init();
    virtual bool                Shutdown();
    virtual bool                CreatePipeline() = 0;

    // Getters
    VkPipeline                  GetPipeline() const;
    VkPipelineLayout            GetPipelineLayout() const;

    // UniBuff getters          // #TECHS Should allow use uni buffers arrays
    virtual VkBuffer            UniBuffer() const { return nullptr; }
    virtual VkDeviceMemory      UniBufferMemory() const { return nullptr; }
    virtual size_t              GetSingleUniBuffObjSize() const { return 0; }

    // Image getters
    virtual void                GetImageSamplerPairs(std::vector<TImgSampler>& out_pairs) const {}

    // Buffers handle
    virtual bool                CreateRenderObjects() { return true; }
    virtual void                DestroyRenderObjects() {}

protected:
    // Pure virtual
    virtual void                GetPipelineLayoutDesc(VkPipelineLayoutCreateInfo& pipelineLayoutInfo) = 0;
    virtual void                GetShadersDesc(SShaderParams& params) = 0;
    virtual uint32_t            GetRenderSubpassIndex() const = 0;

    // Get Pipeline description
    virtual void                GetVertexInputDesc(VkPipelineVertexInputStateCreateInfo& /*vertexInputInfo*/) {}
    virtual void                GetInputAssemblyDesc(VkPipelineInputAssemblyStateCreateInfo& /*inputAssembly*/) {}
    virtual void                GetViewportDesc(VkPipelineViewportStateCreateInfo& /*viewportState*/) {}
    virtual void                GetRasterizerDesc(VkPipelineRasterizationStateCreateInfo& /*rasterizer*/) {}
    virtual void                GetMultisamplingDesc(VkPipelineMultisampleStateCreateInfo& /*multisampling*/) {}
    virtual void                GetDepthStencilDesc(VkPipelineDepthStencilStateCreateInfo& /*depthStencil*/) {}
    virtual void                GetColorBlendDesc(VkPipelineColorBlendStateCreateInfo& /*colorBlending*/) {}
    virtual void                GetDynamicStateDesc(VkPipelineDynamicStateCreateInfo& /*dynamicState*/) {}

    // Create
    virtual bool                CreatePipelineLayout();

    // Pipeline
    VkPipelineLayout            m_PipelineLayout = nullptr;
    VkPipeline                  m_Pipeline = nullptr;

    // Renderer
    CVulkanRenderer*            m_Renderer = nullptr;

    // Parent Tech
    ITechnique*                 m_Parent = nullptr;
};

