#pragma once
#include "ITechnique.h"

struct SObjUniBuffer
{
    glm::mat4 obj_world;
    float tex_mul;
};

struct BaseVertex
{
    BaseVertex() = default;

    glm::vec3 pos;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription m_BindingDesc;
    static std::array<VkVertexInputAttributeDescription, 2> m_AttributeDesc;

    static VkVertexInputBindingDescription* GetBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2>* GetAttributeDescriptions();
};

class CBaseTechnique : public ITechnique
{
public:
    CBaseTechnique();
    ~CBaseTechnique();

    // UniBuff getters
    size_t GetSingleUniBuffObjSize() const { return sizeof(SObjUniBuffer); }
    VkBuffer BaseObjUniBuffer() const { return m_BaseObjUniBuffer; }
    VkDeviceMemory BaseObjUniBufferMemory() const { return m_BaseObjUniBufferMemory; }

    // Buffers handle
    bool CreateUniBuffers();

protected:
    void GetVertexInputDesc(VkPipelineVertexInputStateCreateInfo& vertexInputInfo) override;
    void GetShadersDesc(SShaderParams& params) override;

private:
    VkBuffer m_BaseObjUniBuffer = nullptr;
    VkDeviceMemory m_BaseObjUniBufferMemory = nullptr;
};

