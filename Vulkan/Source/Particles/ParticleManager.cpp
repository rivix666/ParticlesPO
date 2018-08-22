#include "stdafx.h"
#include "ParticleManager.h"
#include "Emitters/IEmitter.h"
#include "../Techs/TechniqueManager.h"
#include "../DescriptorManager.h"

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
    uint32_t techs = g_Engine->TechMgr()->TechniquesCount();
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
    uint32_t old_num = m_ParticlesNum;

    // Clear old counters
    m_ParticlesNum = 0;
    for (auto& ui : m_Tech2PCount)
        ui = 0;

    for (auto emit : m_Emitters)
    {
        emit->Simulate();
        m_Tech2PCount[emit->TechId()] += emit->ParticlesCount();
        m_ParticlesNum += emit->ParticlesCount();
    }
    if (m_ParticlesNum != old_num)
        g_Engine->RequestCommandBufferReset();
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
    for (uint32_t i = 0; i < m_Tech2Emi.size(); i++)
    {
        if (m_Tech2Emi[i].empty() || m_Tech2PCount[i] < 1)
            continue;

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

            memcpy(data, emi->ParticlesData(), (size_t)bufferSize);
            offset += emi->ParticlesSize();
        }
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

    uint32_t offset = 0;
    auto tech_mgr = g_Engine->TechMgr();
    for (uint32_t i = 0; i < m_Tech2Emi.size(); i++)
    {
        if (m_Tech2Emi[i].empty())
            continue;

        auto tech = tech_mgr->GetTechnique(i);

        // Bind Pipeline
        vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, tech->GetPipeline());

        // Bind Descriptor Sets
        std::vector<VkDescriptorSet> desc_sets = { 
            g_Engine->DescMgr()->DescriptorSet((uint32_t)EDescSetRole::GENERAL), 
            g_Engine->DescMgr()->DescriptorSet((uint32_t)EDescSetRole::PARTICLES),
            g_Engine->DescMgr()->DescriptorSet((uint32_t)EDescSetRole::DEPTH)
        };
        vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, tech->GetPipelineLayout(), 0, (uint32_t)desc_sets.size(), desc_sets.data(), 0, nullptr);  //#UNI_BUFF

        // Bind Vertex buffer
        VkBuffer vertexBuffers[] = { m_VertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd_buff, 0, 1, vertexBuffers, offsets);
        vkCmdDraw(cmd_buff, (uint32_t)(m_Tech2PCount[i]), 1, offset, 0);

        offset += m_Tech2PCount[i];
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
            uint32_t old_size = m_Emitters.size();
            m_Emitters.resize(id + 1);
            for (uint32_t i = old_size; i < m_Emitters.size(); i++)
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

bool CParticleManager::CreateUniBuffers()
{
    // Create Uni Buffers
    PrepareVectors();
    for (uint32_t i = 0; i < g_Engine->TechMgr()->TechniquesCount(); i++)
    {
        if (!CreateTechUniBuffer(i))
            return false;
    }

    // Register as Descriptors
    if (!RegisterUniBuffs())
        return false;

    return true;
}

void CParticleManager::PrepareVectors()
{
    if (!m_UniBuffs.empty())
    {
        ReleaseUniBuffers();
    }

    m_UniBuffs.resize((size_t)g_Engine->TechMgr()->TechniquesCount());
    m_UniBuffsMem.resize((size_t)g_Engine->TechMgr()->TechniquesCount());
}

bool CParticleManager::CreateTechUniBuffer(const uint32_t& tech_id)
{
    // Create Buffer
    size_t minUboAlignment = g_Engine->Renderer()->MinUboAlignment();
    double size = ceil((double)sizeof(SParticleTechUniBuffer) / (double)minUboAlignment);
    VkDeviceSize bufferSize = minUboAlignment * size * g_Engine->TechMgr()->TechniquesCount();

    if (!g_Engine->Renderer()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniBuffs[tech_id], m_UniBuffsMem[tech_id]))
        return false;

    SParticleTechUniBuffer ub = {};
    FetchTechUboData(tech_id, ub);

    // Copy Data into it
    void* data;
    vkMapMemory(g_Engine->Device(), m_UniBuffsMem[tech_id], 0, sizeof(ub), 0, &data);
    memcpy(data, &ub, sizeof(ub));
    vkUnmapMemory(g_Engine->Device(), m_UniBuffsMem[tech_id]);
    return true;
}

void CParticleManager::FetchTechUboData(const uint32_t& tech_id, SParticleTechUniBuffer& out)
{
    auto pbt = dynamic_cast<CParticleBaseTechnique*>(g_Engine->TechMgr()->GetTechnique(tech_id));
    if (pbt)
    {
        out = pbt->GetParticleUniBuffData();
    }
}

void CParticleManager::ReleaseUniBuffers()
{
    for (auto& ubo : m_UniBuffs)
    {
        if (ubo)
        {
            vkDestroyBuffer(g_Engine->Renderer()->GetDevice(), ubo, nullptr);
        }
    }

    for (auto& mem : m_UniBuffsMem)
    {
        if (mem)
        {
            vkFreeMemory(g_Engine->Renderer()->GetDevice(), mem, nullptr);
        }
    }
}

bool CParticleManager::RegisterUniBuffs()
{
    // Store sizes of individual elements
    size_t offset = 0;
    std::vector<size_t> sizes;
    for (size_t i = 0; i < g_Engine->TechMgr()->TechniquesCount(); i++)
    {
        sizes.push_back(g_Engine->Renderer()->GetUniBuffObjSize(sizeof(SParticleTechUniBuffer)));
    }

    // Register images array in Descriptor Set
    if (!g_Engine->Renderer()->DescMgr()->RegisterDescriptor(
        m_UniBuffs, sizes,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        (uint32_t)EDescSetRole::PARTICLES,
        g_Engine->Renderer()->DescMgr()->GetNextFreeLocationId((uint32_t)EDescSetRole::PARTICLES)))
        return false;

    return true;
}