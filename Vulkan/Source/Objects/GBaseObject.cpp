#include "stdafx.h"
#include "GBaseObject.h"
#include "../Techs/BaseTechnique.h"
#include "../Techs/TechniqueManager.h"

uint CGBaseObject::s_TechId = UINT_MAX;

CGBaseObject::CGBaseObject(const EBaseObjInitType& type)
    : m_Type(type)
{
    m_TechId = s_TechId; //#TECH keipskie rozwiazanie ale na razie na szybko jest
    InitVectors(type);
}

CGBaseObject::CGBaseObject(const EBaseObjInitType& type, const SObjMtxInitParams& params)
    : IGObject(params)
    , m_Type(type)
{
    m_TechId = s_TechId; //#TECH keipskie rozwiazanie ale na razie na szybko jest
    InitVectors(type);
}

CGBaseObject::CGBaseObject(const std::vector<uint16_t>& indices, const std::vector<BaseVertex>& vertices, const SObjMtxInitParams& params)
    : IGObject(params)
    , m_Indices(indices)
    , m_Vertices(vertices)
    , m_Type(EBaseObjInitType::USER_DEFINED)
{
    m_TechId = s_TechId; //#TECH keipskie rozwiazanie ale na razie na szybko jest
}

CGBaseObject::~CGBaseObject()
{
    ShutdownPhysXObj();
}

bool CGBaseObject::InitPhysXObj()
{
    switch (m_Type)
    {
    case EBaseObjInitType::PLANE:
    {
        // #PX nie widac na PVD, sprawdzic czy dziala jak beda particle
        m_PxMaterial = g_Engine->PxManager()->SDK()->createMaterial(1.0, 1.0, 0.0);

        glm::vec3 pos(m_WorldMtx[3]);
        physx::PxPlane plane(physx::PxVec3(0.0f, 0.1f, 0.0f), physx::PxVec3(0.0f, 1.0f, 0.0f));
        m_PxActor = PxCreatePlane(*g_Engine->PxManager()->SDK(), plane, *m_PxMaterial);

        physx::PxShape* planeShape = g_Engine->PxManager()->SDK()->createShape(physx::PxPlaneGeometry(), *m_PxMaterial);
        planeShape->setLocalPose(physx::PxTransform(physx::PxVec3(pos.x, pos.y, pos.z)));
        planeShape->setFlag(physx::PxShapeFlag::ePARTICLE_DRAIN, true);
        m_PxActor->attachShape(*planeShape);

        g_Engine->PxManager()->RegisterActor(m_PxActor);
        return true;
    }
    case EBaseObjInitType::BOX:
    {
        m_PxMaterial = g_Engine->PxManager()->SDK()->createMaterial(1.0, 1.0, 0.0);

        glm::vec3 pos(m_WorldMtx[3]);
        physx::PxShape* shape = g_Engine->PxManager()->SDK()->createShape(physx::PxBoxGeometry(1.0f, 1.0f, 1.0f), *m_PxMaterial);
        shape->setFlag(physx::PxShapeFlag::ePARTICLE_DRAIN, true);
        
        physx::PxTransform transform(physx::PxVec3(pos.x, pos.y, pos.z));
        m_PxActor = physx::PxCreateStatic(*g_Engine->PxManager()->SDK(), transform, *shape);

        g_Engine->PxManager()->RegisterActor(m_PxActor);
        return true;
    }
    case EBaseObjInitType::USER_DEFINED:
        return utils::FatalError(g_Engine->Hwnd(), "User defined px actors, were not implemented yet");
    }

    return false;
}

void CGBaseObject::ShutdownPhysXObj()
{
    g_Engine->PxManager()->UnregisterActor(m_PxActor);
    m_PxActor->release();
    m_PxActor = nullptr;
    m_PxMaterial->release();
    m_PxMaterial = nullptr;
}

bool CGBaseObject::CreateBuffers()
{
    if (!m_VertexBuffer)
        CreateVertexBuffer();

    if (!m_IndexBuffer)
        CreateIndexBuffer();

    return true;
}

size_t CGBaseObject::GetVertexSize() const
{
    return sizeof(BaseVertex);
}

size_t CGBaseObject::GetVerticesSize() const
{
    return GetVertexSize() * m_Vertices.size();
}

size_t CGBaseObject::GetIndexSize() const
{
    return sizeof(uint16_t);
}

size_t CGBaseObject::GetIndicesSize() const
{
    return GetIndexSize() * m_Indices.size();
}

uint CGBaseObject::GetIndicesCount() const
{
    return m_Indices.size();
}

uint CGBaseObject::GetVerticesCount() const
{
    return m_Vertices.size();
}

void* CGBaseObject::GetVerticesPtr() 
{
    return &m_Vertices[0];
}

void* CGBaseObject::GetIndicesPtr()
{
    return &m_Indices[0];
}

void* CGBaseObject::GetUniBuffData()
{
    m_UniBuffData.obj_world = m_WorldMtx;
    m_UniBuffData.tex_mul = m_TexMultiplier;
    return &m_UniBuffData;
}

void CGBaseObject::InitVectors(const EBaseObjInitType& type)
{
    switch (type)
    {
    case EBaseObjInitType::PLANE:
    {
        m_Vertices =
        {
            { { -10000.0f, 0.0f, -10000.0f }, { 1.0f, 0.0f } }, // front left
            { {  10000.0f, 0.0f, -10000.0f }, { 0.0f, 0.0f } }, // front right
            { {  10000.0f, 0.0f,  10000.0f }, { 0.0f, 1.0f } }, // back right
            { { -10000.0f, 0.0f,  10000.0f }, { 1.0f, 1.0f } }, // back left
        };
        m_Indices = { 0, 1, 2, 2, 3, 0 };
        m_TexMultiplier = 5000.0f; //#UNI_BUFF
        break;
    }
    case EBaseObjInitType::BOX:
    {
        m_Vertices =
        {
            // DOWN
            { { -1.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } }, // front left 0
            { {  1.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } }, // front right 1
            { { -1.0f, 0.0f,  1.0f }, { 0.0f, 1.0f } }, // back left 2
            { {  1.0f, 0.0f,  1.0f }, { 1.0f, 1.0f } }, // back right 3

            // RIGHT
            { { -1.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } }, // front left 4
            { {  1.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } }, // front right 5
            { { -1.0f, 2.0f, -1.0f }, { 0.0f, 1.0f } }, // front  up left 6
            { {  1.0f, 2.0f, -1.0f }, { 1.0f, 1.0f } }, // front up right 7

            // UP
            { { -1.0f, 2.0f, -1.0f }, { 0.0f, 0.0f } }, // front left 8
            { {  1.0f, 2.0f, -1.0f }, { 1.0f, 0.0f } }, // front right 9
            { { -1.0f, 2.0f,  1.0f }, { 0.0f, 1.0f } }, // back left 10
            { {  1.0f, 2.0f,  1.0f }, { 1.0f, 1.0f } }, // back right 11

            // LEFT
            { { -1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }, // front left 12
            { {  1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } }, // front right 13
            { { -1.0f, 2.0f, 1.0f }, { 0.0f, 1.0f } }, // front  up left 14
            { {  1.0f, 2.0f, 1.0f }, { 1.0f, 1.0f } }, // front up right 15

            // FRONT
            { { -1.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } }, // front left 16
            { { -1.0f, 0.0f,  1.0f }, { 1.0f, 0.0f } }, // front right 17
            { { -1.0f, 2.0f, -1.0f }, { 0.0f, 1.0f } }, // front  up left 18
            { { -1.0f, 2.0f,  1.0f }, { 1.0f, 1.0f } }, // front up right 19

            // BACK
            { { 1.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } }, // front left 20
            { { 1.0f, 0.0f,  1.0f }, { 1.0f, 0.0f } }, // front right 21
            { { 1.0f, 2.0f, -1.0f }, { 0.0f, 1.0f } }, // front  up left 22
            { { 1.0f, 2.0f,  1.0f }, { 1.0f, 1.0f } }, // front up right 23
        };
        m_Indices = 
        { 
            // DOWN
            3, 1, 0, 0, 2, 3,

            // RIGHT
            4, 5, 7, 7, 6, 4,

            // UP
            8, 9, 11, 11, 10, 8,

            // LEFT
            15, 13, 12, 12, 14, 15,

            // FRONT
            19, 17, 16, 16, 18, 19,

            // BACK
            20, 21, 23, 23, 22, 20,
        };
        m_TexMultiplier = 1.0f; //#UNI_BUFF
        break;
    }
    }
}

void CGBaseObject::CreateVertexBuffer()
{
    if (m_Vertices.empty())
        return;

    VkDeviceSize bufferSize = GetVerticesSize();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    g_Engine->Renderer()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(g_Engine->Device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, GetVerticesPtr(), (size_t)bufferSize);
    vkUnmapMemory(g_Engine->Device(), stagingBufferMemory);

    g_Engine->Renderer()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);

    g_Engine->Renderer()->CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

    vkDestroyBuffer(g_Engine->Device(), stagingBuffer, nullptr);
    vkFreeMemory(g_Engine->Device(), stagingBufferMemory, nullptr);
}

void CGBaseObject::CreateIndexBuffer()
{
    if (m_Indices.empty())
        return;

    VkDeviceSize bufferSize = GetIndicesSize();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    g_Engine->Renderer()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(g_Engine->Device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, GetIndicesPtr(), (size_t)bufferSize);
    vkUnmapMemory(g_Engine->Device(), stagingBufferMemory);

    g_Engine->Renderer()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

    g_Engine->Renderer()->CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

    vkDestroyBuffer(g_Engine->Device(), stagingBuffer, nullptr);
    vkFreeMemory(g_Engine->Device(), stagingBufferMemory, nullptr);
}