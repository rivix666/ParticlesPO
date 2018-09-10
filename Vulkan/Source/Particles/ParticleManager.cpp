#include "stdafx.h"
#include "ParticleManager.h"
#include "Emitters/BaseEmitter.h"
#include "../Techs/TechniqueManager.h"
#include "../DescriptorManager.h"

SParticleBufferData::SParticleBufferData()
{
    CPU2Px.resize(PARTICLE_BUFF_SIZE);
    Particles.resize(PARTICLE_BUFF_SIZE);
    for (int i = 0; i < PARTICLE_BUFF_SIZE; i++)
    {
        PxPool.push_back(PARTICLE_BUFF_SIZE - i - 1);
        Particles[i].pos = glm::vec3(0.0f, -1.0f, 0.0f);
        Particles[i].dummy_vec = glm::vec3(-1.0f, i, 0.0f);
    }
}

//////////////////////////////////////////////////////////////////////////
CParticleManager::~CParticleManager()
{
    utils::DeletePtrVec(m_Emitters);
}

bool CParticleManager::Init()
{
    // Create Vertex Buffer
    g_Engine->Renderer()->CreateBuffer(
        PARTICLE_VERTEX_BUFF_SIZE,
        /*#COMPUTE*/VK_BUFFER_USAGE_TRANSFER_SRC_BIT |/**/ VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        m_VertexBuffer, 
        m_VertexBufferMemory);

    // Init emitters buffers
    uint32_t techs = g_Engine->TechMgr()->TechniquesCount();
    m_Tech2Emi.resize(techs);
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
    static uint32_t old_num = m_PartData.CPUFree;

    // Emit long emitted particles
    // EmitParticlesInTime();

    // Emit particles from active emitters
    for (const auto& p : m_ActiveEmitters)
    {
        p.first->Emit(p.second);
    }

    // Simulate all particles
    for (auto emit : m_Emitters)
    {
        emit->Simulate();
    }

    // If particle number changed, request command buffer reset
    if (m_PartData.CPUFree != old_num)
        g_Engine->RequestCommandBufferReset();

    // Store particles number after simulate
    old_num = m_PartData.CPUFree;
}

void CParticleManager::UpdateBuffers()
{
    if (m_PartData.CPUFree < 1)
        return;

    // Create staging buffer
    VkDeviceSize bufferSize = /*m_PartData.CPUFree*/ PARTICLE_BUFF_SIZE * sizeof(ParticleVertex); //#COMPUTE
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    g_Engine->Renderer()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // Map staging buffer
    void* data;
    vkMapMemory(g_Engine->Device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_PartData.Particles.data(), (size_t)bufferSize);

    // Unmap staging buffer and copy its data into vertex buffer
    vkUnmapMemory(g_Engine->Device(), stagingBufferMemory);
    g_Engine->Renderer()->CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

    // Destroy staging buffer
    vkDestroyBuffer(g_Engine->Device(), stagingBuffer, nullptr);
    vkFreeMemory(g_Engine->Device(), stagingBufferMemory, nullptr);
}

void CParticleManager::RecordCommandBuffer(VkCommandBuffer& cmd_buff)
{
    if (m_PartData.CPUFree < 1)
        return;

    auto tech_mgr = g_Engine->TechMgr();
    auto g_tech = tech_mgr->GetTechnique(1); //#PARTICLES TMP!!

    // Bind Pipeline
    vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, g_tech->GetPipeline());

    // Bind Descriptor Sets
    std::vector<VkDescriptorSet> desc_sets = {
        g_Engine->DescMgr()->DescriptorSet((uint32_t)EDescSetRole::GENERAL),
        g_Engine->DescMgr()->DescriptorSet((uint32_t)EDescSetRole::PARTICLES),
        g_Engine->DescMgr()->DescriptorSet((uint32_t)EDescSetRole::DEPTH)
    };
    vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, g_tech->GetPipelineLayout(), 0, (uint32_t)desc_sets.size(), desc_sets.data(), 0, nullptr);

    // Bind Vertex buffer
    VkBuffer vertexBuffers[] = { m_VertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd_buff, 0, 1, vertexBuffers, offsets);
    vkCmdDraw(cmd_buff, m_PartData.CPUFree, 1, 0, 0);
}

void CParticleManager::RecordCommandBufferCompute(VkCommandBuffer& cmd_buff)
{
    if (m_SortMethod != ESortType::GPU)
        return;

    if (m_PartData.CPUFree < 1)
        return;

    auto tech_mgr = g_Engine->TechMgr();
    auto c_tech = tech_mgr->GetTechnique(6);

    // Bind Pipeline
    vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, c_tech->GetPipeline());

    // Bind Descriptor Sets
    std::vector<VkDescriptorSet> desc_sets = {
        g_Engine->DescMgr()->DescriptorSet((uint32_t)EDescSetRole::GENERAL),
        g_Engine->DescMgr()->DescriptorSet((uint32_t)EDescSetRole::COMPUTE)
    };
    vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, c_tech->GetPipelineLayout(), 0, (uint32_t)desc_sets.size(), desc_sets.data(), 0, nullptr);

    //////////////////////////////////////////////////////////////////////////
    uint32_t elem_num =  /*PARTICLE_BUFF_SIZE*/ m_PartData.CPUFree;
    uint32_t thread_x = 32;
    //////////////////////////////////////////////////////////////////////////

    // Push constants
    glm::vec4 pc = glm::vec4(glm::normalize(g_Engine->Camera()->CameraPosition()), m_PartData.CPUFree);
    size_t data_size = sizeof(glm::vec4) + sizeof(uint32_t) + sizeof(uint32_t);
    char* data = new char[data_size];
    memcpy(data, &pc, sizeof(glm::vec4));
    for (uint32_t level = 2; level <= 32; level = level * 2)
    {
        // Push constants //#COMPUTE test dx tylko jak neiz adziala to pc poza petla
        memcpy(data + sizeof(glm::vec4), &level, sizeof(uint32_t));
        memcpy(data + sizeof(glm::vec4) + sizeof(uint32_t), &level, sizeof(uint32_t));
        vkCmdPushConstants(cmd_buff, c_tech->GetPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, data_size, data);
        vkCmdDispatch(cmd_buff, elem_num / thread_x, 1, 1);//#COMPUTE
    }

    //for (uint32_t level = (32 * 2); level <= elem_num; level = level * 2)
    //{
    //    uint32_t pc_level = (level / thread_x);
    //    uint32_t pc_level_mask = (level & ~elem_num) / thread_x;
    //
    //    // Push constants //#COMPUTE test dx tylko jak neiz adziala to pc poza petla
    //    memcpy(data + sizeof(glm::vec4), &pc_level, sizeof(uint32_t));
    //    memcpy(data + sizeof(glm::vec4) + sizeof(uint32_t), &pc_level_mask, sizeof(uint32_t));
    //
    //    // Sort the transposed column data
    //    vkCmdDispatch(cmd_buff, elem_num / thread_x, 1, 1);
    //
    //    // Push constants //#COMPUTE test dx tylko jak neiz adziala to pc poza petla
    //    memcpy(data + sizeof(glm::vec4), &thread_x, sizeof(uint32_t));
    //    memcpy(data + sizeof(glm::vec4) + sizeof(uint32_t), &level, sizeof(uint32_t));
    //
    //    // Sort the row data
    //    vkCmdDispatch(cmd_buff, elem_num / thread_x, 1, 1);
    //}


    delete data;
    
    //vkCmdDispatch(cmd_buff, m_PartData.CPUFree / 32 + 1, 1, 1);
}

void CParticleManager::EmitParticles(const uint32_t& idx, const uint32_t& count, const double& in_time /*= 0.0*/)
{
    if (m_Emitters.size() <= idx || count == 0)
        return;

    if (in_time <= 0.0)
    {
        m_Emitters[idx]->Emit(count);
        return;
    }

    m_LongEmitted.insert(new SLongEmit(in_time, idx, count));
}

void CParticleManager::ActivateEmitter(const uint32_t& idx, const uint32_t& count)
{
    auto emit = GetEmitter(idx);
    if (emit)
    {
        m_ActiveEmitters[emit] = count;
    }
}

void CParticleManager::DeactivateEmitter(const uint32_t& idx)
{
    auto emit = GetEmitter(idx);
    if (emit)
    {
        m_ActiveEmitters.erase(emit);
    }
}

void CParticleManager::DeactivateAllEmitters()
{
    m_ActiveEmitters.clear();
}

bool CParticleManager::IsEmitterActive(const uint32_t& idx)
{
    auto emit = GetEmitter(idx);
    if (emit)
    {
        const auto& it = m_ActiveEmitters.find(emit);
        if (it != m_ActiveEmitters.end())
            return true;
    }
    return false;
}

int CParticleManager::RegisterEmitter(CBaseEmitter* emitter, int id /*= -1*/)
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
    emitter->SetEmitterId(tmp_id);
    m_Emitters[tmp_id] = emitter;
    m_Tech2Emi[emitter->TechId()].push_back(emitter);
}

void CParticleManager::UnregisterEmitter(int id)
{
    CBaseEmitter* emit = m_Emitters[id];
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

        // Reset emitter id
        emit->SetEmitterId(UINT32_MAX);
    }
}

int CParticleManager::FindEmitterId(CBaseEmitter* emitter) const
{
    auto it = std::find(m_Emitters.begin(), m_Emitters.end(), emitter);
    if (it != m_Emitters.end())
        return (int)(it - m_Emitters.begin());
    return -1;
}

CBaseEmitter* CParticleManager::GetEmitter(int id) const
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

void CParticleManager::EmitParticlesInTime()
{
    if (m_LongEmitted.empty())
        return;

    std::vector<SLongEmit*> to_release;
    double last_frame_time = g_Engine->LastFrameTime();
    for (auto* it : m_LongEmitted)
    {
        // Calc how many particles emit
        double t_num = std::floor(it->t_left / last_frame_time);
        uint32_t e_num = (uint32_t)std::floor((double)it->p_left / t_num);
        if (e_num > it->p_left)
            e_num = it->p_left;

        // Emit particles
        GetEmitter(it->e_idx)->Emit(e_num);

        // Release if needed
        it->p_left -= e_num;
        it->t_left = (e_num == 0) ? it->t_left + last_frame_time : it->t_left - last_frame_time;
        if (it->p_left == 0 || it->t_left <= 0.0)
            to_release.push_back(it);
    }

    // Release
    for (auto* tr : to_release)
    {
        m_LongEmitted.erase(tr);
        DELETE(tr);
    }
}