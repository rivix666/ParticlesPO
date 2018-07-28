#include "stdafx.h"
#include "IGObject.h"

IGObject::IGObject(const SObjMtxInitParams& params)
{
    Translate(params.translation);
    Scale(params.scale);
}

IGObject::~IGObject()
{
    CleanupBuffers();
}

bool IGObject::CreateBuffers()
{
    // ...
    return false;
}

void IGObject::CleanupBuffers()
{
    if (m_VertexBuffer)
        vkDestroyBuffer(g_Engine->Device(), m_VertexBuffer, nullptr);

    if (m_VertexBufferMemory)
        vkFreeMemory(g_Engine->Device(), m_VertexBufferMemory, nullptr);

    if (m_IndexBuffer)
        vkDestroyBuffer(g_Engine->Device(), m_IndexBuffer, nullptr);

    if (m_IndexBufferMemory)
        vkFreeMemory(g_Engine->Device(), m_IndexBufferMemory, nullptr);
}