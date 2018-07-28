#pragma once
#include "IGObject.h"
#include "BaseTechnique.h"

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
    bool     InitPhysXObj() override;
    void     ShutdownPhysXObj() override;

    bool     CreateBuffers() override;

    size_t   GetVertexSize() const override;
    size_t   GetVerticesSize() const override;
    size_t   GetIndexSize() const override;
    size_t   GetIndicesSize() const override;

    uint     GetIndicesCount() const override;
    uint     GetVerticesCount() const override;
    void*    GetVerticesPtr() override;
    void*    GetIndicesPtr() override;

    void*    GetUniBuffData() override;

    static uint s_TechId; //#TECH keipskie rozwiazanie ale na razie na szybko jest

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
    physx::PxRigidStatic* m_PxActor;
    physx::PxMaterial* m_PxMaterial;

    // Misc
    EBaseObjInitType m_Type;
};