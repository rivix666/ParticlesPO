#pragma once
#include "IGObject.h"
#include "../Techs/BaseTechnique.h"

enum class EBaseObjInitType
{
    PLANE = 0,
    BOX,
    USER_DEFINED,

    _COUNT_
};

class CGBaseObject : public IGObject
{
public:
    CGBaseObject() = default;
    CGBaseObject(const EBaseObjInitType& type);
    CGBaseObject(const EBaseObjInitType& type, const SObjMtxInitParams& params);
    CGBaseObject(const std::vector<uint16_t>& indices, const std::vector<BaseVertex>& vertices, const SObjMtxInitParams& params);
    ~CGBaseObject();

    // Derived from IGObject   
    bool InitPhysXObj() override;
    void ShutdownPhysXObj() override;

    bool CreateBuffers() override;

    size_t GetVertexSize() const override;
    size_t GetVerticesSize() const override;
    size_t GetIndexSize() const override;
    size_t GetIndicesSize() const override;

    uint32_t GetIndicesCount() const override;
    uint32_t GetVerticesCount() const override;
    void* GetVerticesPtr() override;
    void* GetIndicesPtr() override;

    void* GetUniBuffData() override;

    void Translate(const glm::vec3& pos);

    static uint32_t s_TechId; //#TECH keipskie rozwiazanie ale na razie na szybko jest

protected:
    void InitVectors(const EBaseObjInitType& type);
    void CreateVertexBuffer();
    void CreateIndexBuffer();

private:
    // Buffers data
    SObjUniBuffer m_UniBuffData;
    std::vector<uint16_t> m_Indices;
    std::vector<BaseVertex> m_Vertices;

    // PhysX
    physx::PxActor* m_PxActor = nullptr;
    physx::PxMaterial* m_PxMaterial = nullptr;

    // Misc
    EBaseObjInitType m_Type;
};