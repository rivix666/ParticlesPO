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

    // Init emitters buffers
    uint techs = g_Engine->TechMgr()->TechniquesCount();
    m_Tech2Emi.resize(techs);
    m_Tech2PCount.resize(techs);

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
    m_ParticlesNum = 0;
    for (auto emit : m_Emitters)
    {
        emit->Simulate();
        m_Tech2PCount[emit->TechId()] += emit->ParticlesCount();
        m_ParticlesNum += emit->ParticlesCount();
    }
}

void CParticleManager::UpdateBuffers()
{
    if (m_ParticlesNum < 1)
        return;

    // Create staging buffer
    VkDeviceSize bufferSize = m_ParticlesNum * IEmitter::SingleParticleSize();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    g_Engine->Renderer()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // Map staging buffer
    void* data;
    vkMapMemory(g_Engine->Device(), stagingBufferMemory, 0, bufferSize, 0, &data);

    size_t offset = 0;
    for (uint i = 0; i < m_Tech2Emi.size(); i++)
    {
        if (m_Tech2Emi[i].empty() || m_Tech2PCount[i] < 1)
        {
            // Always clear on end
            m_Tech2PCount[i] = 0;
            continue;
        }

        for (auto* emi : m_Tech2Emi[i])
        {
            if (!emi)
                continue;

            #ifdef _DEBUG
            if (offset + emi->ParticlesSize() > bufferSize)
            {
                utils::FatalError(g_Engine->Hwnd(), "Particles won't fit in the buffer!");
            }
            #endif

            // #PARTICLES sprawdzic czy zadziala
            memcpy(data, emi->ParticlesData(), (size_t)bufferSize);
           // memcpy((void*)(((char*)data) + offset), emi->ParticlesData(), emi->ParticlesSize());
            offset += emi->ParticlesSize();
        }

        // Always clear on end
        m_Tech2PCount[i] = 0;
    }

    // Unmap staging buffer and copy its data into vertex buffer
    vkUnmapMemory(g_Engine->Device(), stagingBufferMemory);
    g_Engine->Renderer()->CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

    // Destroy staging buffer
    vkDestroyBuffer(g_Engine->Device(), stagingBuffer, nullptr);
    vkFreeMemory(g_Engine->Device(), stagingBufferMemory, nullptr);
}

void CParticleManager::RecordCommandBuffer(VkCommandBuffer& cmd_buff)
{
    if (m_ParticlesNum < 1)
        return;

    auto tech_mgr = g_Engine->TechMgr();
    for (uint i = 0; i < m_Tech2Emi.size(); i++)
    {
        if (m_Tech2Emi[i].empty())
            continue;

        auto tech = tech_mgr->GetTechnique(i);
        vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, tech->GetPipeline());

        // #PARTICLES tymczasowo by zobaczyc czy rysuje particle
        //////////////////////////////////////////////////////////////////////////
        // Bind Vertex buffer
        VkBuffer vertexBuffers[] = { m_VertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd_buff, 0, 1, vertexBuffers, offsets);

        // Prepare uni buff offset
        uint32_t uni_offset = tech->GetUniBuffObjOffset() * i;
        
        vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, tech->GetPipelineLayout(), 0, 1, &g_Engine->Renderer()->m_DescriptorSet, 1, &uni_offset);
        vkCmdDraw(cmd_buff, static_cast<uint32_t>(m_ParticlesNum), 1, 0, 0);
        //////////////////////////////////////////////////////////////////////////


        // #PARTICLES przy moim aktualnym rozumowaniu nie przejdzie,
        // musia³bym nagrywaæ od nowa co klatkê command buffer (w sumie mo¿na to sprawdziæ jak wp³ynie na performance z reset command buffer, ale raczej kiepsko)
        // a musia³bym to robiæ ze wzglêdu na zmieniaj¹c¹ siê liczbê particli per ró¿ne techniki, chyba ¿e przypisaæ do ka¿dej techniki sta³¹ liczbê particli to by przesz³o
        // albo zrobiæ oddzielne vertex buffery per technika, wtedy te¿ ka¿dy particle móg³by korzystaæ z wa³snych vertexów
    }
}

int CParticleManager::RegisterEmitter(IEmitter* emitter, int id /*= -1*/)
{
    if (!emitter)
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
    m_Tech2Emi[emitter->TechId()].push_back(emitter);
}

void CParticleManager::UnregisterEmitter(int id)
{
    IEmitter* emit = m_Emitters[id];
    if (emit)
    {
        // Remove from emitters
        m_Emitters.erase(m_Emitters.begin() + id, m_Emitters.begin() + id + 1);

        // Remove from tech map
        auto& tech_vec = m_Tech2Emi[emit->TechId()];
        auto it = std::find(tech_vec.begin(), tech_vec.end(), emit);
        if (it != tech_vec.end())
        {
            tech_vec.erase(it, it + 1);
        }
    }
}

int CParticleManager::FindEmitterId(IEmitter* emitter) const
{
    auto it = std::find(m_Emitters.begin(), m_Emitters.end(), emitter);
    if (it != m_Emitters.end())
        return (int)(it - m_Emitters.begin());
    return -1;
}

IEmitter* CParticleManager::GetEmitter(int id) const
{
    return m_Emitters[id];
}
