#include "stdafx.h"
#include "ParticleManager.h"
#include "Emitters/IEmitter.h"
#include "../Techs/TechniqueManager.h"

CParticleManager::CParticleManager()
{
}

CParticleManager::~CParticleManager()
{
    utils::DeletePtrVec(m_Emitters);
}

bool CParticleManager::Init()
{
    // Create Vertex Buffer
    g_Engine->Renderer()->CreateBuffer(
        PARTICLE_BUFF_SIZE * IEmitter::SingleParticleSize(), 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        m_VertexBuffer, 
        m_VertexBufferMemory);
    return true;
}

bool CParticleManager::Shutdown()
{
    // Cleanup buffers
    if (m_VertexBuffer)
        vkDestroyBuffer(g_Engine->Device(), m_VertexBuffer, nullptr);

    if (m_VertexBufferMemory)
        vkFreeMemory(g_Engine->Device(), m_VertexBufferMemory, nullptr);

    return false;
}

void CParticleManager::Simulate()
{
    for (auto emit : m_Emitters)
    {
        emit->Simulate();
    }
}

void CParticleManager::UpdateBuffers()
{
    // #PARTICLE_MGR
    // stworzyc staging buffer do komunikacji z cpu i vertex poprawny w systemie. sprawdziæ czy lepiej stworzyc jeden staging z max size i aktualizowaæ czy mo¿e mniejsze na bie¿¹co

    //     if (m_Vertices.empty())
    //         return;
    // 
    //     VkDeviceSize bufferSize = GetVerticesSize();
    // 
    //     VkBuffer stagingBuffer;
    //     VkDeviceMemory stagingBufferMemory;
    //     g_Engine->Renderer()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    // 
    //     void* data;
    //     vkMapMemory(g_Engine->Device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    //     memcpy(data, GetVerticesPtr(), (size_t)bufferSize);
    //     vkUnmapMemory(g_Engine->Device(), stagingBufferMemory);
    // 
    //     g_Engine->Renderer()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);
    //     g_Engine->Renderer()->CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);
    // 
    //     vkDestroyBuffer(g_Engine->Device(), stagingBuffer, nullptr);
    //     vkFreeMemory(g_Engine->Device(), stagingBufferMemory, nullptr);
}

void CParticleManager::RecordCommandBuffer(VkCommandBuffer& cmd_buff)
{

}

int CParticleManager::RegisterEmitter(IEmitter* emitter, const char* name, int id /*= -1*/)
{
    if (FindEmitterId(emitter) < 0)
        return -1;

    // Prepare slot for new emitter
    int tmp_id = -1;
    if (id < 0)
    {
        m_Emitters.push_back(nullptr);
        tmp_id = m_Emitters.size() - 1;
    }
    else
    {
        if (id >= m_Emitters.size())
        {
            uint old_size = m_Emitters.size();
            m_Emitters.resize(id + 1);
            for (uint i = old_size; i < m_Emitters.size(); i++)
            {
                m_Emitters[i] = nullptr;
            }
        }
        tmp_id = id;
    }

    // Register emitter
    m_Emitters[tmp_id] = emitter;
    m_EmittersIdMap[emitter] = name;
}

void CParticleManager::UnregisterEmitter(int id)
{
    IEmitter* emit = m_Emitters[id];
    m_Emitters.erase(m_Emitters.begin() + id, m_Emitters.begin() + id + 1);
    m_EmittersIdMap.erase(emit);
}

int CParticleManager::FindEmitterId(IEmitter* emitter) const
{
    auto it = std::find(m_Emitters.begin(), m_Emitters.end(), emitter);
    if (it != m_Emitters.end())
        return (int)(it - m_Emitters.begin());
    return -1;
}

int CParticleManager::FindEmitterId(const char* name) const
{
    auto it = std::find_if(m_EmittersIdMap.begin(), m_EmittersIdMap.end(), [=](std::pair<IEmitter*, const char*> p) { return strcmp(name, p.second) == 0; });
    if (it != m_EmittersIdMap.end())
        return FindEmitterId((*it).first);
    return -1;
}

IEmitter* CParticleManager::GetEmitter(int id) const
{
    return m_Emitters[id];
}
