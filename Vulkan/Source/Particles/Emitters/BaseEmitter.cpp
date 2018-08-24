#include "stdafx.h"
#include "BaseEmitter.h"
#include "../ParticleManager.h"

CBaseEmitter::CBaseEmitter(const uint32_t& tech_id, const uint32_t& buffer_size)
    : m_TechId(tech_id)
    , m_BuffSize(buffer_size)
{
    // Prepare vectors
    m_Px2CPU.resize(buffer_size);
    for (int i = 0; i < buffer_size; i++)
    {
        m_PxPool.push_back(buffer_size - 1 - i);
    }

    // Prepare PhysX
    m_PxScene = g_Engine->PxManager()->Scene();
    m_PxParticles = CreateParticleSystem();
}

void CBaseEmitter::Emit(const uint32_t& count_per_second)
{
    std::vector<uint32_t> indexes;
    std::vector<physx::PxVec3> directions;
    CreateParticles(indexes, directions, count_per_second);
    if (indexes.size() > 0)
        SendParticlesToPhysX(indexes, directions);
}

void CBaseEmitter::Simulate()
{
    std::vector<uint32_t> to_release;
    physx::PxParticleReadData* rd = m_PxParticles->lockParticleReadData();
    if (rd->validParticleRange > 0)
    {
        // Iterate over valid particle bitmap
        auto& data = ParticleMgr()->BuffData();
        physx::PxStrideIterator<const physx::PxVec3> positions(rd->positionBuffer);
        physx::PxStrideIterator<const physx::PxParticleFlags> particleFlags(rd->flagsBuffer);
        for (uint32_t w = 0; w <= (rd->validParticleRange - 1) >> 5; w++)
        {
            for (uint32_t b = rd->validParticleBitmap[w]; b; b &= b - 1)
            {
                uint32_t index = (w << 5 | LowestBitSet(b));

                // Access particle position
                const physx::PxVec3& position = positions[index];
                int cpu_idx = m_Px2CPU[index];
                UpdateParticlePos(cpu_idx, position);
                MakeStepInParticleLife(cpu_idx);

                // Check if particle should be released
                if (data.Particles[cpu_idx].life < MIN_LIFE
                    || (m_UseParticleDrain && (particleFlags[index] & physx::PxParticleFlag::eCOLLISION_WITH_DRAIN)))
                {
                    // Mark particle for delete/replace
                    data.Particles[cpu_idx].life = RESET_LIFE;
                    to_release.push_back(index);
                }
            }
        }
    }
    rd->unlock();

    // We need to release after unlock, otherwise assert
    ReleaseParticles(to_release);
}

CParticleManager* CBaseEmitter::ParticleMgr()
{
    return g_Engine->ParticleMgr();
}

physx::PxParticleSystem* CBaseEmitter::CreateParticleSystem()
{
    physx::PxParticleSystem* ps = g_Engine->PxManager()->SDK()->createParticleSystem(m_BuffSize);
    if (!ps)
    {
        utils::FatalError(g_Engine->Hwnd(), "Failed to create PxParticleSystem");
    }

    PrepareParticleSystem(ps);
    m_PxScene->addActor(*ps);

#if PX_SUPPORT_GPU_PHYSX
    // Check gpu flags after adding to scene, cpu fallback might have been used.
    m_RunOnGpu = m_RunOnGpu && (ps->getParticleBaseFlags() & physx::PxParticleBaseFlag::eGPU);
#endif
    return ps;
}

void CBaseEmitter::PrepareParticleSystem(physx::PxParticleSystem* ps)
{
    ps->setGridSize(3.0f);
    ps->setMaxMotionDistance(0.1f);
    ps->setRestOffset(0.0143f);
    ps->setContactOffset(0.0143f * 2);
    ps->setDamping(0.0f);
    ps->setRestitution(0.2f);
    ps->setDynamicFriction(0.05f);
    ps->setParticleReadDataFlag(physx::PxParticleReadDataFlag::eVELOCITY_BUFFER, true);
#if PX_SUPPORT_GPU_PHYSX
    ps->setParticleBaseFlag(physx::PxParticleBaseFlag::eGPU, m_RunOnGpu);
#endif
}

void CBaseEmitter::CreateParticles(std::vector<uint32_t>& idx_out, std::vector<physx::PxVec3>& dir_out, const uint32_t& emit_count)
{
    for (uint32_t i = 0; i < emit_count; i++)
    {
        if (m_UsedParticles >= m_BuffSize)
            break;

        int px_idx = ReserveSlotInBuffer();
        idx_out.push_back(px_idx);

        float x, y, z;
        GenerateRandomXYZ(x, y, z);
        dir_out.push_back(physx::PxVec3(x, y, z));

        m_UsedParticles++;
    }
}

void CBaseEmitter::ReleaseParticles(const std::vector<uint32_t>& index_buff)
{
    if (index_buff.size() > 0)
    {
        auto& data = ParticleMgr()->BuffData();
        for (int i = 0; i < index_buff.size(); i++)
        {
            m_UsedParticles--;
            m_PxPool.push_back(index_buff[i]);

            uint32_t hole = m_Px2CPU[index_buff[i]];
            uint32_t last = --data.CPUFree;

            if (hole == last)
            {
                data.Particles[last].life = -1.f;
                data.Particles[last].pos = glm::vec3(0.0f, -1.0f, 0.0f);
                continue;
            }

            data.Particles[hole] = data.Particles[last];

            data.Particles[last].life = -1;
            data.Particles[last].pos = glm::vec3(0.0f, -1.0f, 0.0f);

            data.CPU2Px[hole] = data.CPU2Px[last];

            auto hole_emit = ParticleMgr()->GetEmitter(data.CPU2Px[hole].EmitId);
            hole_emit->m_Px2CPU[data.CPU2Px[hole].PartId] = hole;
        }
        physx::PxStrideIterator<const uint32_t> it_index_buffer(index_buff.data(), sizeof(uint32_t));
        m_PxParticles->releaseParticles(index_buff.size(), it_index_buffer);
    }
}

void CBaseEmitter::MakeStepInParticleLife(const uint32_t& cpu_idx)
{
    auto& data = ParticleMgr()->BuffData();
    data.Particles[cpu_idx].life -= 0.001f;
}

void CBaseEmitter::UpdateParticlePos(const uint32_t& cpu_idx, const physx::PxVec3& new_pos)
{
    auto& data = ParticleMgr()->BuffData();
    data.Particles[cpu_idx].pos.x = new_pos.x;
    data.Particles[cpu_idx].pos.y = new_pos.y;
    data.Particles[cpu_idx].pos.z = new_pos.z;
}

const uint32_t& CBaseEmitter::ReserveSlotInBuffer()
{
    auto& data = ParticleMgr()->BuffData();
    uint32_t cpu_idx = data.CPUFree++;
    uint32_t px_idx = m_PxPool.back();
    m_PxPool.pop_back();

    m_Px2CPU[px_idx] = cpu_idx;
    data.CPU2Px[cpu_idx].PartId = px_idx;
    data.CPU2Px[cpu_idx].EmitId = m_EmitterId;

    data.Particles[cpu_idx].life = RESET_LIFE;
    data.Particles[cpu_idx].tech_id = m_TechId;

    return px_idx;
}

void CBaseEmitter::SendParticlesToPhysX(const std::vector<uint32_t>& idx, const std::vector<physx::PxVec3>& dir)
{
    physx::PxParticleCreationData particleCreationData;
    physx::PxVec3 tempEmitPos(m_Pos.x, m_Pos.y, m_Pos.z);

    particleCreationData.numParticles = idx.size();
    particleCreationData.indexBuffer = physx::PxStrideIterator<const uint32_t>(idx.data());
    particleCreationData.positionBuffer = physx::PxStrideIterator<const physx::PxVec3>(&tempEmitPos, 0);
    particleCreationData.velocityBuffer = physx::PxStrideIterator<const physx::PxVec3>(dir.data());

    m_PxParticles->createParticles(particleCreationData);
}

uint32_t CBaseEmitter::LowestBitSet(uint32_t x)
{
    if (x == 0)
        return 0;

    uint32_t result = 0;
    while ((x & 1) == 0)
    {
        x >>= 1;
        result++;
    }
    return result;
}

void CBaseEmitter::GenerateRandomXYZ(float& x, float& y, float& z)
{
    x = (m_Direction.x + float(rand() % 5000 - 2500) / float(-200)) * (float)(rand() % 100);
    y = (m_Direction.y + float(rand() % 5000 - 2500) / float(-200)) * (float)(rand() % 100);
    z = (m_Direction.z + float(rand() % 5000 - 2500) / float(-200)) * (float)(rand() % 100);
}