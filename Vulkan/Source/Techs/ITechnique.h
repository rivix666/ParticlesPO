#pragma once
#include "../Utils/ShaderUtils.h"

class CVulkanRenderer;

class ITechnique
{
public:
    ITechnique() = default;
    virtual ~ITechnique();

    // Typedefs
    typedef std::pair<VkImageView, VkSampler> TImgSampler;

    // Init
    virtual bool Init();
    virtual bool Shutdown();
    virtual bool CreateGraphicsPipeline();

    // Getters
    VkPipeline GetPipeline() const { return m_GraphicsPipeline; }
    VkPipelineLayout GetPipelineLayout() const  { return m_PipelineLayout; }

    // UniBuff getters
    virtual VkBuffer UniBuffer() const { return nullptr; }
    virtual VkDeviceMemory UniBufferMemory() const { return nullptr; }
    virtual size_t GetUniBuffObjOffset() const;
    virtual size_t GetSingleUniBuffObjSize() const { return 0; }

    // Image getters
    virtual void GetImageSamplerPairs(std::vector<TImgSampler>& out_pairs) const {}

    // Buffers handle
    virtual bool CreateRenderObjects() { return true; }
    virtual void DestroyRenderObjects() {}

protected:
    // Pure virtual
    virtual void GetVertexInputDesc(VkPipelineVertexInputStateCreateInfo& vertexInputInfo) = 0;
    virtual void GetShadersDesc(SShaderParams& params) = 0;

    // Get Pipeline description
    virtual void GetInputAssemblyDesc(VkPipelineInputAssemblyStateCreateInfo& inputAssembly);
    virtual void GetViewportDesc(VkPipelineViewportStateCreateInfo& viewportState);
    virtual void GetRasterizerDesc(VkPipelineRasterizationStateCreateInfo& rasterizer);
    virtual void GetMultisamplingDesc(VkPipelineMultisampleStateCreateInfo& multisampling);
    virtual void GetDepthStencilDesc(VkPipelineDepthStencilStateCreateInfo& depthStencil);
    virtual void GetColorBlendDesc(VkPipelineColorBlendStateCreateInfo& colorBlending);
    virtual void GetDynamicStateDesc(VkPipelineDynamicStateCreateInfo& dynamicState);
    virtual void GetPipelineLayoutDesc(VkPipelineLayoutCreateInfo& pipelineLayoutInfo);

    // Create
    virtual bool CreatePipelineLayout();

    // Pipeline
    VkPipelineLayout m_PipelineLayout = nullptr;
    VkPipeline       m_GraphicsPipeline = nullptr;

    // Renderer
    CVulkanRenderer* m_Renderer = nullptr;
};

