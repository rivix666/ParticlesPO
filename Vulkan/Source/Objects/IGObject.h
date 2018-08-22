#pragma once
#include "../Techs/ITechnique.h"

struct SObjMtxInitParams
{
    SObjMtxInitParams() = default;
    SObjMtxInitParams(const glm::vec3& t = glm::vec3(0.0f, 0.0f, 0.0f), const glm::vec3& s = glm::vec3(1.0f, 1.0f, 1.0f))
        : translation(t), scale(s) {}
    ~SObjMtxInitParams() = default;

    glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f); 
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
};

class IGObject
{
public:
    IGObject() = default;
    IGObject(const SObjMtxInitParams& params);
    ~IGObject();

    // Init PhysX
    virtual bool InitPhysXObj() { return true; }
    virtual void ShutdownPhysXObj() {}

    // Getters/Setters
    virtual size_t GetVertexSize() const = 0;
    virtual size_t GetVerticesSize() const = 0;
    virtual size_t GetIndexSize() const = 0;
    virtual size_t GetIndicesSize() const = 0;
    virtual uint32_t   GetIndicesCount() const = 0;
    virtual uint32_t   GetVerticesCount() const = 0;

    virtual void*  GetVerticesPtr() = 0;
    virtual void*  GetIndicesPtr() = 0;

    virtual void*  GetUniBuffData() { return nullptr; };

    void SetTechId(uint32_t tech_id) { m_TechId = tech_id; }
    uint32_t TechniqueId() const { return m_TechId; }

    void SetWorldMtx(const glm::mat4& mtx) { m_WorldMtx = mtx; }
    const glm::mat4& WorldMtx() const { return m_WorldMtx; }

    // Matrix operations
    void Translate(const glm::vec3& pos) { m_WorldMtx = glm::translate(m_WorldMtx, pos); }
    void Rotate(const float& angle, const glm::vec3& axes) { m_WorldMtx = glm::rotate(m_WorldMtx, angle, axes); }
    void Scale(const glm::vec3& scale) { m_WorldMtx = glm::scale(m_WorldMtx, scale);}

    // Buffers getters
    const VkBuffer& IndexBuffer() const { return m_IndexBuffer; }
    const VkBuffer& VertexBuffer() const { return m_VertexBuffer; }
    const VkDeviceMemory& IndexBufferMem() const { return m_IndexBufferMemory; }
    const VkDeviceMemory& VertexBufferMem() const { return m_VertexBufferMemory; }

    // Buffers handle
    virtual bool CreateBuffers();
    virtual void CleanupBuffers();

protected:
    // Object Tech Id
    uint32_t m_TechId = UINT_MAX;

    // Matrix
    glm::mat4 m_WorldMtx = glm::mat4(1.0f);

    // Buffers
    VkBuffer m_IndexBuffer = nullptr;
    VkBuffer m_VertexBuffer = nullptr;
    VkDeviceMemory m_IndexBufferMemory = nullptr;
    VkDeviceMemory m_VertexBufferMemory = nullptr;

    // Misc
    float m_TexMultiplier = 1.0f; // Texture UV multiplier
};

